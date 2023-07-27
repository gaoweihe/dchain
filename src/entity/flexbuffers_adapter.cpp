#include "flexbuffers_adapter.hpp"

namespace tomchain {

std::shared_ptr<BLSSigShare> flexbuffers_adapter<BLSSigShare>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    auto g1_blob = map["g1"].AsBlob();
    assert(g1_blob.size() == 96); 
    auto g1_data = g1_blob.data(); 
    std::vector<uint8_t> g1_bv(g1_data, g1_data + 96);
    auto g1_rptr = (libff::alt_bn128_G1 *)(g1_bv.data());
    auto g1 = std::make_shared<libff::alt_bn128_G1>(*g1_rptr); 

    auto hint = map["hint"].AsString().str();
    auto signer_index = map["signer_index"].AsUInt64();
    auto t = map["t"].AsUInt64();
    auto n = map["n"].AsUInt64();

    auto sig_share = std::make_shared<BLSSigShare>(g1, hint, signer_index, t, n);
    return sig_share; 
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<BLSSigShare>::to_bytes(const BLSSigShare& sig_share) {
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
        fbb.UInt("signer_index", signer_index);
        fbb.UInt("t", t);
        fbb.UInt("n", n);
    });

    fbb.Finish();

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer());
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<BlockVote>::to_bytes(const BlockVote& vote) {
    flexbuffers::Builder fbb;
    fbb.Map([&]() {
        fbb.UInt("block_id", vote.block_id_);
        fbb.UInt("voter_id", vote.voter_id_);
        fbb.Blob("sig_share", *flexbuffers_adapter<BLSSigShare>::to_bytes(*(vote.sig_share_)));
    }); 
    fbb.Finish(); 

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer()); 
}

std::shared_ptr<BlockVote> flexbuffers_adapter<BlockVote>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    auto ss_blob = map["sig_share"].AsBlob();
    std::vector<uint8_t> ss_bv(ss_blob.data(), ss_blob.data() + ss_blob.size()); 
    auto sig_share = flexbuffers_adapter<BLSSigShare>::from_bytes(std::make_shared<std::vector<uint8_t>>(ss_bv));
    
    BlockVote vote;
    vote.block_id_ = map["block_id"].AsUInt64();
    vote.voter_id_ = map["voter_id"].AsUInt64();
    vote.sig_share_ = sig_share;

    std::shared_ptr<BlockVote> sp_vote = std::make_shared<BlockVote>(vote);

    return sp_vote; 
}

}