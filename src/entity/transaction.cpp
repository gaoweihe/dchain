#include "transaction.hpp"

namespace tomchain {

Transaction::Transaction(
    uint64_t id, 
    uint64_t sender, 
    uint64_t receiver, 
    uint64_t value, 
    uint64_t fee
) : id_(id), 
    sender_(sender), 
    receiver_(receiver), 
    value_(value), 
    fee_(fee) 
{

}

Transaction::Transaction(const Transaction& tx)
{
    this->id_ = tx.id_; 
    this->sender_ = tx.sender_; 
    this->receiver_ = tx.receiver_; 
    this->value_ = tx.value_; 
    this->fee_ = tx.fee_;
}

Transaction::Transaction()
{
    
}

Transaction::~Transaction()
{
    
}

}