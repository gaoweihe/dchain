#ifndef TC_BLOCK_HDR
#define TC_BLOCK_HDR

#include <vector>
#include <memory> 
#include <istream>
#include <ostream>

#include "c_plus_plus_serializer.h"

#include "transaction.hpp"

namespace tomchain {

class BlockHeader {
public: 
    uint64_t id_;
    uint64_t base_id_;

    friend std::ostream& operator<<(std::ostream &out, Bits<class BlockHeader & > obj); 
    friend std::istream& operator>>(std::istream &in, Bits<class BlockHeader &> obj); 
}; 

/**
 * @brief Defines the block data structure in TomChain. 
 * 
 */
class Block {
public: 
    Block(); 
    Block(uint64_t id, uint64_t base_id);
    virtual ~Block(); 

public: 
    friend std::ostream& operator<<(std::ostream &out, Bits<class Block & > obj)
    {
        out << 
            bits(obj.t.header_) << 
            bits(obj.t.tx_vec_);
        return out;
    }

    friend std::istream& operator>>(std::istream &in, Bits<class Block &> obj)
    {
        in >> 
            bits(obj.t.header_) >> 
            bits(obj.t.tx_vec_);
        return in;
    }

public: 
    void insert(std::shared_ptr<Transaction> tx);

public: 
    BlockHeader header_; 
    std::vector<
        std::shared_ptr<Transaction>
    > tx_vec_; 
};

};

#endif /* TC_BLOCK_HDR */