#include "block.hpp"

namespace tomchain {

std::ostream& operator<<(std::ostream &out, Bits<class BlockHeader & > obj)
{
    out << 
        bits(obj.t.id_) << 
        bits(obj.t.base_id_);
    return out;
}

std::istream& operator>>(std::istream &in, Bits<class BlockHeader &> obj)
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

}