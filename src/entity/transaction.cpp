#include "transaction.hpp"

namespace tomchain {

Transaction::Transaction(uint64_t sender, uint64_t receiver, uint64_t value) 
    : sender(evmc::address(sender)), receiver(evmc::address(receiver)), value(value)
{

}

Transaction::~Transaction()
{
    
}

}