#include <vector>

#include "transaction.hpp"

namespace tomchain {

/**
 * @brief Defines the block data structure in TomChain. 
 * 
 */
class Block {
public: 
    Block();
    virtual ~Block(); 

public: 
    std::vector<Transaction> txVec; 
};

};