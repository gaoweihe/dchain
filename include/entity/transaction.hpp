#include <evmc/evmc.hpp>

namespace tomchain {

/**
 * @brief Define the data structure of Transaction in TomChain. 
 * 
 */
class Transaction {
public: 
    Transaction();
    virtual ~Transaction();

public: 
    evmc::address sender; 
    evmc::address receiver; 
    evmc_uint256be value; 
};

}