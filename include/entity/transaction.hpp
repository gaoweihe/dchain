#include <evmc/evmc.hpp>

namespace tomchain {

/**
 * @brief Define the data structure of Transaction in TomChain. 
 * 
 */
class Transaction {
public: 
    Transaction(uint64_t id_, uint64_t sender, uint64_t receiver, uint64_t value);
    virtual ~Transaction();

public: 
    uint64_t id_;
    evmc::address sender_; 
    evmc::address receiver_; 
    uint64_t value_;
};
}