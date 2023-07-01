#include "block.hpp"
#include "msgpack_packer.hpp"

#include <nlohmann/json.hpp>

extern std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

// std::ostream& operator<<(std::ostream &out, Bits<class BlockHeader&> obj)
// {
//     out << 
//         bits(obj.t.id_) << 
//         bits(obj.t.base_id_);
//     return out;
// }

// std::istream& operator>>(std::istream &in, Bits<class BlockHeader&> obj)
// {
//     in >>
//         bits(obj.t.id_) >> 
//         bits(obj.t.base_id_);
//     return in;
// }

Block::Block() { }

Block::Block(uint64_t id, uint64_t base_id)
    :header_({id, base_id})
{
    this->tx_vec_ = std::vector<
        std::shared_ptr<Transaction>
    >(); 
}

Block::~Block()
{
    
}

void Block::insert(std::shared_ptr<Transaction> tx)
{
    this->tx_vec_.push_back(tx); 
}

std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> Block::get_sha256() 
{
    std::vector<unsigned char> hash(picosha2::k_digest_size);

    std::stringstream ss; 
    ss << bits(*this); 
    std::string ss_str = ss.str(); 

    picosha2::hash256(ss_str.begin(), ss_str.end(), hash.begin(), hash.end()); 

    std::array<uint8_t, picosha2::k_digest_size> hash_arr;
    std::copy_n(hash.begin(), picosha2::k_digest_size, hash_arr.begin()); 

    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> spHashArr = 
        std::make_shared<std::array<uint8_t, picosha2::k_digest_size>>(hash_arr); 
    return spHashArr;
}

// std::ostream& operator<<(std::ostream &out, Bits<class Block&> obj)
// {
//     out << 
//         bits(obj.t.header_) << 
//         bits(obj.t.tx_vec_) << 
//         bits(obj.t.votes_);
//     return out;
// }

// std::istream& operator>>(std::istream &in, Bits<class Block&> obj)
// {
//     in >> 
//         bits(obj.t.header_) >> 
//         bits(obj.t.tx_vec_) >>
//         bits(obj.t.votes_);
//     return in;
// }

BlockVote::BlockVote() { }

// std::ostream& operator<<(std::ostream &out, Bits<class BlockVote&> obj)
// {
//     out << 
//         bits((uint32_t)(obj.t.sig_share_->getSignerIndex())) <<
//         bits((uint32_t)(obj.t.sig_share_->getRequiredSigners())) <<
//         bits((uint32_t)(obj.t.sig_share_->getTotalSigners())) <<
//         bits(*(obj.t.sig_share_->toString()));
//     return out;
// }

// std::istream& operator>>(std::istream &in, Bits<class BlockVote&> obj)
// {
//     std::string sig_share_str;
//     uint32_t signer_index; 
//     uint32_t required_signers;
//     uint32_t total_signers; 
//     in >> 
//         bits(signer_index) >> 
//         bits(required_signers) >> 
//         bits(total_signers) >> 
//         bits(sig_share_str);
//     obj.t.sig_share_ = std::make_shared<BLSSigShare>(
//         BLSSigShare(
//             std::make_shared<std::string>(sig_share_str), 
//             (size_t)signer_index, 
//             (size_t)required_signers, 
//             (size_t)total_signers
//         )
//     );
//     return in;
// }

}