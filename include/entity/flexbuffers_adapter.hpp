#pragma once
#ifndef TC_FLEXBUFFERS_ADAPTER
#define TC_FLEXBUFFERS_ADAPTER

#include <flatbuffers/flexbuffers.h>
#include <memory>
#include <vector>
#include "libBLS/libBLS.h"
#include "libBLS/bls/BLSSigShare.h"

namespace tomchain {

class flexbuffers_adapter {
public:

static std::shared_ptr<BLSSigShare> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes); 
static std::shared_ptr<std::vector<uint8_t>> to_bytes(const BLSSigShare& sig_share); 

};

}

#endif
