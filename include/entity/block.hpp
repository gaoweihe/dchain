#ifndef TC_BLOCK_HDR
#define TC_BLOCK_HDR

#include "libBLS/libBLS.h"
#include "libBLS/bls/BLSSigShare.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "picosha2.h"
#include "msgpack.hpp"

#include <vector>
#include <memory> 
#include <istream>
#include <ostream>
#include <string>

#include "transaction.hpp"

namespace tomchain {

class BlockVote; 

class BlockHeader {
public: 
    BlockHeader(); 
    BlockHeader(uint64_t id, uint64_t base_id);
    BlockHeader(const BlockHeader& bh); 

public: 
    uint64_t id_;
    uint64_t base_id_;

    MSGPACK_DEFINE(
        id_,
        base_id_
    );
}; 

class BlockVote {
public: 
    BlockVote(); 
    BlockVote(const BlockVote& bv); 

public: 
    uint64_t block_id_; 
    uint64_t voter_id_; 
    std::shared_ptr<BLSSigShare> sig_share_; 
}; 

/**
 * @brief Defines the block data structure in TomChain. 
 * 
 */
class Block {
public: 
    Block(); 
    Block(uint64_t id, uint64_t base_id);
    Block(const Block& block); 
    virtual ~Block(); 

public: 
    void insert(std::shared_ptr<Transaction> tx);
    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> get_sha256(); 
    bool is_vote_enough(const uint64_t target_num) const; 
    void merge_votes(const uint64_t target_num); 

    // server id starts from one 
    uint64_t get_server_id(uint64_t server_count) const; 

public: 
    BlockHeader header_; 
    std::vector<
        std::shared_ptr<Transaction>
    > tx_vec_; 
    std::map<uint64_t, std::shared_ptr<BlockVote>> votes_; 
    std::shared_ptr<BLSSignature> tss_sig_;
};

};

#endif /* TC_BLOCK_HDR */