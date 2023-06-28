#ifndef TC_BLOCK_HDR
#define TC_BLOCK_HDR

#include <vector>
#include <memory> 
#include <istream>
#include <ostream>

#include "c_plus_plus_serializer.h"

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
    friend std::ostream& operator<<(std::ostream &out, Bits<class Block & > obj)
    {
        out << 
            bits(obj.t.id_) << 
            bits(obj.t.base_id_) <<
            bits(obj.t.tx_vec_);
        return (out);
    }

    friend std::istream& operator>>(std::istream &in, Bits<class Block &> obj)
    {
        in >> 
            bits(obj.t.id_) >> 
            bits(obj.t.base_id_) >>
            bits(obj.t.tx_vec_);
        return (in);
    }

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