#include "libBLS/libBLS.h"
#include "block.hpp"
#include "msgpack_adapter.hpp"
#include "spdlog/spdlog.h"

using namespace tomchain; 

int main() {
    DKGBLSWrapper dkg_obj(1, 1);
    std::shared_ptr<std::vector<libff::alt_bn128_Fr>> sshares = dkg_obj.createDKGSecretShares(); 
    BLSPrivateKeyShare skey_share(sshares->at(0), 1, 1);
    BLSPublicKeyShare pkey_share(sshares->at(0), 1, 1); 

    spdlog::info("initialize context"); 

    std::vector<unsigned char> hash(picosha2::k_digest_size);
    std::string str{"hello world"};
    picosha2::hash256(str.begin(), str.end(), hash.begin(), hash.end()); 
    std::array<uint8_t, picosha2::k_digest_size> hash_arr;
    std::copy_n(hash.begin(), picosha2::k_digest_size, hash_arr.begin()); 
    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> spHashArr = 
        std::make_shared<std::array<uint8_t, picosha2::k_digest_size>>(hash_arr); 

    // signer index starts from one
    std::shared_ptr<BLSSigShare> sig_share = skey_share.sign(spHashArr, 1);

    spdlog::info("sign message"); 

    msgpack::sbuffer b;
    msgpack::pack(b, sig_share); 

    spdlog::info("stub 1"); 
 
    std::string ser_str = sbufferToString(b);
    msgpack::sbuffer des_b = stringToSbuffer(ser_str);

    spdlog::info("stub 2"); 

    auto oh = msgpack::unpack(des_b.data(), des_b.size());
    auto f2 = oh->as<std::shared_ptr<BLSSigShare>>();

    spdlog::info("{}", f2->getRequiredSigners()); 

    BlockVote vote;
    vote.block_id_ = 1; 
    vote.sig_share_ = f2; 

    msgpack::sbuffer b2;
    msgpack::pack(b2, std::make_shared<BlockVote>(vote));

    std::string ser_str2 = sbufferToString(b2);
    msgpack::sbuffer des_b2 = stringToSbuffer(ser_str2);

    auto oh2 = msgpack::unpack(b2.data(), b2.size());
    auto f3 = oh2->as<std::shared_ptr<BlockVote>>();

    spdlog::info("{}", f3->block_id_);

    Block block;
    block.header_.id_ = 1;
    block.votes_.insert(
        std::make_pair(
            1,
            f3
        )
    ); 

    msgpack::sbuffer b3;
    msgpack::pack(b3, std::make_shared<Block>(block));

    std::string ser_str3 = sbufferToString(b3);
    msgpack::sbuffer des_b3 = stringToSbuffer(ser_str3);

    auto oh3 = msgpack::unpack(b3.data(), b3.size());
    std::shared_ptr<Block> f4 = oh3->as<std::shared_ptr<Block>>();

    spdlog::info("{}", f4->header_.id_);
    for (auto vote_iter = f4->votes_.begin(); vote_iter != f4->votes_.end(); vote_iter++)
    {
        spdlog::info("{}", vote_iter->first);
        spdlog::info("{}", vote_iter->second->block_id_);
        spdlog::info("{}", vote_iter->second->sig_share_->getRequiredSigners());
    }
    
    uint64_t t = f4->votes_.find(1)->second->sig_share_->getRequiredSigners();
    spdlog::info("t: {}", t); 

    return 0; 
}



