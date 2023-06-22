#include <evmc/evmc.hpp>

namespace tomchain {

/**
 * @brief Define the data structure of Transaction in TomChain. 
 * 
 */
class Transaction {
public: 
    Transaction(uint64_t sender, uint64_t receiver, uint64_t value);
    virtual ~Transaction();

public: 
    evmc::address sender; 
    evmc::address receiver; 
    uint64_t value;
};
}