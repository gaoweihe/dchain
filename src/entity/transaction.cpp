#include "transaction.hpp"

namespace tomchain {

Transaction::Transaction(uint64_t id, uint64_t sender, uint64_t receiver, uint64_t value) 
    : id_(id), sender_(evmc::address(sender)), receiver_(evmc::address(receiver)), value_(value)
{

}

Transaction::~Transaction()
{
    
}

}