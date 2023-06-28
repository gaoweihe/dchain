#ifndef TC_BLOCK_HDR
#define TC_BLOCK_HDR

#include <vector>
#include <memory> 

#include "transaction.hpp"

namespace tomchain {

/**
 * @brief Defines the block data structure in TomChain. 
 * 
 */
class Block {
public: 
    Block(uint64_t id, uint64_t base_id_);
    virtual ~Block(); 

public: 
    void insert(std::shared_ptr<Transaction> tx);

public: 
    uint64_t id_;
    uint64_t base_id_;
    std::vector<
        std::shared_ptr<Transaction>
    > tx_vec_; 
};

};

#endif /* TC_BLOCK_HDR */