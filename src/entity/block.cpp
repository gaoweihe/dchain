#include "block.hpp"

namespace tomchain {

Block::Block(uint64_t id)
    :id_(id)
{
    this->tx_vec_ = std::vector<Transaction>(); 
}

Block::~Block()
{
    
}

void Block::insert(const Transaction tx)
{
    this->tx_vec_.push_back(tx); 
}

}