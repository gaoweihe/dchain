#include <vector>

#include "transaction.hpp"

namespace tomchain {

/**
 * @brief Defines the block data structure in TomChain. 
 * 
 */
class Block {
public: 
    Block(uint64_t id);
    virtual ~Block(); 

public: 
    void insert(const Transaction tx);

public: 
    uint64_t id_;
    std::vector<Transaction> tx_vec_; 
};

};