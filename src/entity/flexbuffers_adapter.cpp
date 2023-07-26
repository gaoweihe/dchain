#include "flexbuffers_adapter.hpp"

namespace tomchain {

std::shared_ptr<BLSSigShare> from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    auto g1_blob = map["g1"].AsBlob();
    assert(g1_blob.size() == 96); 
    auto g1_data = g1_blob.data(); 
    std::vector<uint8_t> g1_bv(g1_data, g1_data + 96);
    auto g1_rptr = (libff::alt_bn128_G1 *)(g1_bv.data());
    auto g1 = std::make_shared<libff::alt_bn128_G1>(*g1_rptr); 

    auto hint = map["hint"].AsString().str();
    auto signer_index = map["signer_index"].AsInt64();
    auto t = map["t"].AsInt64();
    auto n = map["n"].AsInt64();

    auto sig_share = std::make_shared<BLSSigShare>(g1, hint, signer_index, t, n);
    return sig_share; 
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter::to_bytes(const BLSSigShare& sig_share) {
    std::shared_ptr<libff::alt_bn128_G1> g1 = sig_share.getSigShare();
    std::vector<uint8_t> g1_bv((uint8_t *)(g1.get()), (uint8_t *)(g1.get()) + 96);
    static_assert(sizeof(libff::alt_bn128_G1) == 96); 
    assert(g1_bv.size() == 96); 

    std::string hint = sig_share.getHint(); 
    uint64_t signer_index = sig_share.getSignerIndex();
    uint64_t t = sig_share.getRequiredSigners(); 
    uint64_t n = sig_share.getTotalSigners(); 

    flexbuffers::Builder fbb;
    fbb.Map([&]() {
        fbb.Blob("g1", g1_bv); 
        fbb.String("hint", hint); 
        fbb.Int("signer_index", signer_index);
        fbb.Int("t", t);
        fbb.Int("n", n);
    });

    fbb.Finish();

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer());
}

}