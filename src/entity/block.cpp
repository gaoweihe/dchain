#include "block.hpp"
#include "msgpack_adapter.hpp"

#include <nlohmann/json.hpp>

extern std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

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

    msgpack::sbuffer b;
    msgpack::pack(b, *this); 
    std::string ss_str = sbufferToString(b);

    picosha2::hash256(ss_str.begin(), ss_str.end(), hash.begin(), hash.end()); 

    std::array<uint8_t, picosha2::k_digest_size> hash_arr;
    std::copy_n(hash.begin(), picosha2::k_digest_size, hash_arr.begin()); 

    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> spHashArr = 
        std::make_shared<std::array<uint8_t, picosha2::k_digest_size>>(hash_arr); 
    return spHashArr;
}

BlockVote::BlockVote() { }

}