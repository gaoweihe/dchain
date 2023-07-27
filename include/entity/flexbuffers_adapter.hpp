#pragma once
#ifndef TC_FLEXBUFFERS_ADAPTER
#define TC_FLEXBUFFERS_ADAPTER

#include <flatbuffers/flexbuffers.h>
#include <memory>
#include <vector>
#include "libBLS/libBLS.h"
#include "libBLS/bls/BLSSigShare.h"

#include "block.hpp"

namespace tomchain {

template<typename T> 
struct flexbuffers_adapter {
    static std::shared_ptr<T> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const T& sig_share); 
};

template<> 
struct flexbuffers_adapter<BLSSigShare> {
    static std::shared_ptr<BLSSigShare> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const BLSSigShare& sig_share); 
};

template<> 
struct flexbuffers_adapter<BlockVote> {
    static std::shared_ptr<BlockVote> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const BlockVote& vote); 
};

template<> 
struct flexbuffers_adapter<BLSSignature> {
    static std::shared_ptr<BLSSignature> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const BLSSignature& vote); 
};

template<> 
struct flexbuffers_adapter<BlockHeader> {
    static std::shared_ptr<BlockHeader> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const BlockHeader& vote); 
};

template<> 
struct flexbuffers_adapter<Transaction> {
    static std::shared_ptr<Transaction> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const Transaction& vote); 
};

template<> 
struct flexbuffers_adapter<Block> {
    static std::shared_ptr<Block> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
    static std::shared_ptr<std::vector<uint8_t>> to_bytes(const Block& vote); 
};

}

#endif
