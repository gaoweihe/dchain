#include "block.hpp"

#include <nlohmann/json.hpp>

extern std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

std::ostream& operator<<(std::ostream &out, Bits<class BlockHeader&> obj)
{
    out << 
        bits(obj.t.id_) << 
        bits(obj.t.base_id_);
    return out;
}

std::istream& operator>>(std::istream &in, Bits<class BlockHeader&> obj)
{
    in >>
        bits(obj.t.id_) >> 
        bits(obj.t.base_id_);
    return in;
}

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

std::ostream& operator<<(std::ostream &out, Bits<class Block&> obj)
{
    out << 
        bits(obj.t.header_) << 
        bits(obj.t.tx_vec_);
    return out;
}

std::istream& operator>>(std::istream &in, Bits<class Block&> obj)
{
    in >> 
        bits(obj.t.header_) >> 
        bits(obj.t.tx_vec_);
    return in;
}

BlockVote::BlockVote() { }

std::ostream& operator<<(std::ostream &out, Bits<class BlockVote&> obj)
{
    out << 
        bits(obj.t.header_) << 
        bits((uint32_t)(obj.t.sig_share_->getSignerIndex())) <<
        bits(*(obj.t.sig_share_->toString()));
    return out;
}

std::istream& operator>>(std::istream &in, Bits<class BlockVote&> obj)
{
    std::string sig_share_str;
    uint32_t signer_index; 
    in >> 
        bits(obj.t.header_) >> 
        bits(signer_index) >> 
        bits(sig_share_str);
    obj.t.sig_share_ = std::make_shared<BLSSigShare>(
        BLSSigShare(
            std::make_shared<std::string>(sig_share_str), 
            (size_t)signer_index, 
            (size_t)((*::conf_data)["client-count"]), 
            (size_t)((*::conf_data)["client-count"])
        )
    );
    return in;
}

}