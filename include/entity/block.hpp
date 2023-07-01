#ifndef TC_BLOCK_HDR
#define TC_BLOCK_HDR

#include "c_plus_plus_serializer.h"
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
// typedef oneapi::tbb::concurrent_hash_map<
//     uint64_t, std::shared_ptr<BlockVoteSerde>
// > BlockVoteSerdeCHM; 

class BlockHeader {
public: 
    uint64_t id_;
    uint64_t base_id_;

    // friend std::ostream& operator<<(std::ostream &out, Bits<class BlockHeader&> obj); 
    // friend std::istream& operator>>(std::istream &in, Bits<class BlockHeader&> obj); 

    MSGPACK_DEFINE(
        id_,
        base_id_
    );
}; 

// typedef BlockHeaderSerde BlockHeader; 

class BlockVote {
public: 
    BlockVote(); 

public: 
    uint64_t block_id_; 
    std::shared_ptr<BLSSigShare> sig_share_; 

public: 
    // friend std::ostream& operator<<(std::ostream &out, Bits<class BlockVote&> obj);
    // friend std::istream& operator>>(std::istream &in, Bits<class BlockVote&> obj);
}; 

// class BlockVoteSerde {
// public: 
//     uint32_t signer_index;
//     uint32_t t; 
//     uint32_t n; 
//     std::shared_ptr<std::string> sig_share_str; 

//     BlockVoteSerde(const BlockVote& bv)
//     {
//         signer_index = bv.sig_share_->getSignerIndex(); 
//         t = bv.sig_share_->getRequiredSigners(); 
//         n = bv.sig_share_->getTotalSigners(); 
//         sig_share_str = bv.sig_share_->toString(); 
//     }

//     std::shared_ptr<BlockVote> to_bv()
//     {
//         BlockVote bv; 
//         BLSSigShare ss(sig_share_str, signer_index, t, n); 
//         bv.sig_share_ = std::make_shared(ss);

//         return std::make_shared(bv); 
//     }
     

//     MSGPACK_DEFINE(
//         signer_index,
//         t,
//         n, 
//         sig_share_str 
//     );
// }; 

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
    // friend std::ostream& operator<<(std::ostream &out, Bits<class Block&> obj); 
    // friend std::istream& operator>>(std::istream &in, Bits<class Block&> obj); 

public: 
    void insert(std::shared_ptr<Transaction> tx);
    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> get_sha256(); 

public: 
    BlockHeader header_; 
    std::vector<
        std::shared_ptr<Transaction>
    > tx_vec_; 
    std::map<uint64_t, std::shared_ptr<BlockVote>> votes_; 
    std::shared_ptr<BLSSignature> tss_sig_;
};

// class BlockSerde {
// public: 
//     BlockHeaderSerde header; 
//     std::vector<
//         std::shared_ptr<Transaction>
//     > tx_vec; 
//     BlockVoteSerdeCHM votes; 
//     uint32_t t; 
//     uint32_t n; 
//     std::shared_ptr<std::string> tss_sig;

//     BlockSerde(Block& block)
//     {
//         header = BlockHeaderSerde(block.header_); 
//         tx_vec = block.tx_vec_; 
//         votes = block.votes_; 
//         t = block.tss_sig_->getRequiredSigners(); 
//         n = block.tss_sig_->getTotalSigners(); 
//         tss_sig = block.tss_sig_->toString(); 
//     }

//     std::shared_ptr<Block> to_block()
//     {
//         Block block; 
//         block.header_ = header; 
//         block.tx_vec_ = tx_vec; 
//         block.votes_ = votes; 
        
//         BLSSignature sig(tss_sig, t, n);
//         block.tss_sig_ = std::make_shared(sig); 
//     }

//     MSGPACK_DEFINE(
//         header, 
//         tx_vec, 
//         votes, 
//         t, 
//         n, 
//         tss_sig
//     ); 
// }; 

};

#endif /* TC_BLOCK_HDR */