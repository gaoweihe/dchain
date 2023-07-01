#include "libBLS/libBLS.h"
#include "block.hpp"
#include "msgpack_packer.hpp"
#include "spdlog/spdlog.h"

int main() {
    DKGBLSWrapper dkg_obj(1, 1);
    std::shared_ptr<std::vector<libff::alt_bn128_Fr>> sshares = dkg_obj.createDKGSecretShares(); 
    BLSPrivateKeyShare skey_share(sshares->at(0), 1, 1);
    BLSPublicKeyShare pkey_share(sshares->at(0), 1, 1); 

    std::vector<unsigned char> hash(picosha2::k_digest_size);
    std::string str{"hello world"};
    picosha2::hash256(str.begin(), str.end(), hash.begin(), hash.end()); 
    std::array<uint8_t, picosha2::k_digest_size> hash_arr;
    std::copy_n(hash.begin(), picosha2::k_digest_size, hash_arr.begin()); 
    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> spHashArr = 
        std::make_shared<std::array<uint8_t, picosha2::k_digest_size>>(hash_arr); 

    // signer index starts from one
    std::shared_ptr<BLSSigShare> sig_share = skey_share.sign(spHashArr, 1);

    msgpack::sbuffer b;
    msgpack::pack(b, sig_share);

    auto oh = msgpack::unpack(b.data(), b.size());
    auto f2 = oh->as<BLSSigShare>();

    spdlog::info("{}", f2.getRequiredSigners()); 

    return 0; 
}



