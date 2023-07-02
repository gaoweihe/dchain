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

Transaction::Transaction()
{
    
}

Transaction::~Transaction()
{
    
}

}