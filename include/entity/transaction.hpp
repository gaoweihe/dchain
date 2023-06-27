#ifndef TC_TRNASACTION_HDR
#define TC_TRNASACTION_HDR

#include <evmc/evmc.hpp>

namespace tomchain {

/**
 * @brief Define the data structure of Transaction in TomChain. 
 * 
 */
class Transaction {
public: 
    Transaction(
        uint64_t id, 
        uint64_t sender, 
        uint64_t receiver, 
        uint64_t value, 
        uint64_t fee);
    virtual ~Transaction();

public: 
    /**
     * @brief Transaction ID
     * 
     */
    uint64_t id_;

    /**
     * @brief Transaction sender address
     * 
     */
    evmc::address sender_; 

    /**
     * @brief Transaction receiver address
     * 
     */
    evmc::address receiver_; 

    /**
     * @brief Transaction value
     * 
     */
    uint64_t value_;
    
    /**
     * @brief Transaction fee
     * 
     */
    uint64_t fee_; 
};
}

#endif /* TC_TRNASACTION_HDR */