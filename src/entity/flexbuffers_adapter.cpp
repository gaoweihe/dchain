#include "flexbuffers_adapter.hpp"
#include <string>

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

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<BLSSignature>::to_bytes(const BLSSignature& sig) {
    flexbuffers::Builder fbb;

    std::shared_ptr<libff::alt_bn128_G1> g1 = sig.getSig();
    std::vector<uint8_t> g1_bv((uint8_t *)(g1.get()), (uint8_t *)(g1.get()) + 96);
    static_assert(sizeof(libff::alt_bn128_G1) == 96); 
    assert(g1_bv.size() == 96); 

    fbb.Map([&]() {
        fbb.Blob("sig", g1_bv);
        fbb.String("hint", sig.getHint());
        fbb.UInt("t", sig.getRequiredSigners());
        fbb.UInt("n", sig.getTotalSigners());
    }); 
    fbb.Finish(); 

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer()); 
}

std::shared_ptr<BLSSignature> flexbuffers_adapter<BLSSignature>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    auto g1_blob = map["sig"].AsBlob();
    assert(g1_blob.size() == 96);
    auto g1_data = g1_blob.data();
    std::vector<uint8_t> g1_bv(g1_data, g1_data + 96);
    auto g1_rptr = (libff::alt_bn128_G1 *)(g1_bv.data());
    auto g1 = std::make_shared<libff::alt_bn128_G1>(*g1_rptr);

    auto hint = map["hint"].AsString().str();
    auto t = map["t"].AsUInt64();
    auto n = map["n"].AsUInt64();

    auto sig = std::make_shared<BLSSignature>(g1, hint, t, n);

    return sig; 
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<BlockHeader>::to_bytes(const BlockHeader& bh) {
    flexbuffers::Builder fbb;

    fbb.Map([&]() {
        fbb.UInt("id", bh.id_);
        fbb.UInt("base_id", bh.base_id_);
    }); 
    fbb.Finish(); 

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer()); 
}

std::shared_ptr<BlockHeader> flexbuffers_adapter<BlockHeader>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    std::shared_ptr<BlockHeader> bh = std::make_shared<BlockHeader>();
    bh->id_ = map["id"].AsUInt64();
    bh->base_id_ = map["base_id"].AsUInt64();

    return bh; 
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<Transaction>::to_bytes(const Transaction& tx) {
    flexbuffers::Builder fbb;

    fbb.Map([&]() {
        fbb.UInt("id", tx.id_);
        fbb.UInt("sender", tx.sender_);
        fbb.UInt("receiver", tx.receiver_);
        fbb.UInt("value", tx.value_);
        fbb.UInt("fee", tx.fee_);
    }); 
    fbb.Finish(); 

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer()); 
}

std::shared_ptr<Transaction> flexbuffers_adapter<Transaction>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    std::shared_ptr<Transaction> tx = std::make_shared<Transaction>(
        map["id"].AsUInt64(),
        map["sender"].AsUInt64(),
        map["receiver"].AsUInt64(),
        map["value"].AsUInt64(),
        map["fee"].AsUInt64()
    );

    return tx; 
}

std::shared_ptr<std::vector<uint8_t>> flexbuffers_adapter<Block>::to_bytes(const Block& block) {
    flexbuffers::Builder fbb;

    auto bh_bv = flexbuffers_adapter<BlockHeader>::to_bytes(block.header_);

    fbb.Map([&]() {
        fbb.Blob("header", *bh_bv);
        fbb.Vector("tx_vec", [&]() {
            for (auto tx : block.tx_vec_) {
                auto tx_bv = flexbuffers_adapter<Transaction>::to_bytes(*tx);
                fbb.Blob(*tx_bv);
            }
        });
        fbb.Map("votes", [&]() {
            for (auto iter : block.votes_) {
                auto vote_bv = flexbuffers_adapter<BlockVote>::to_bytes(*(iter.second));
                fbb.Blob(std::to_string(iter.first).c_str(), *vote_bv);
            }
        });
        auto tss_sig_bv = flexbuffers_adapter<BLSSignature>::to_bytes(*(block.tss_sig_));
        fbb.Blob("tss_sig", *tss_sig_bv);

    }); 
    fbb.Finish(); 

    return std::make_shared<std::vector<uint8_t>>(fbb.GetBuffer()); 
}

std::shared_ptr<Block> flexbuffers_adapter<Block>::from_bytes(std::shared_ptr<std::vector<uint8_t>> bytes) {
    auto map = flexbuffers::GetRoot(*bytes).AsMap(); 

    auto bh_blob = map["header"].AsBlob();
    std::vector<uint8_t> bh_bv(bh_blob.data(), bh_blob.data() + bh_blob.size()); 
    auto bh = flexbuffers_adapter<BlockHeader>::from_bytes(std::make_shared<std::vector<uint8_t>>(bh_bv));

    std::vector<std::shared_ptr<Transaction>> tx_vec;
    auto tx_vec_fb = map["tx_vec"].AsVector();
    for (size_t i = 0; i < tx_vec_fb.size(); i++)
    {
        auto tx_blob = tx_vec_fb[i].AsBlob();
        auto tx_bv = std::vector<uint8_t>(tx_blob.data(), tx_blob.data() + tx_blob.size());
        auto tx = flexbuffers_adapter<Transaction>::from_bytes(std::make_shared<std::vector<uint8_t>>(tx_bv));
        tx_vec.push_back(tx);
    }

    std::map<uint64_t, std::shared_ptr<BlockVote>> votes;
    auto votes_fb = map["votes"].AsMap();
    auto keys = votes_fb.Keys(); 
    for (size_t i = 0; i < keys.size(); i++)
    {
        auto key = keys[i].AsString(); 
        auto vote_blob = votes_fb[key.c_str()].AsBlob();
        std::vector<uint8_t> vote_bv(vote_blob.data(), vote_blob.data() + vote_blob.size());
        auto vote = flexbuffers_adapter<BlockVote>::from_bytes(std::make_shared<std::vector<uint8_t>>(vote_bv));
        votes.insert(std::make_pair(std::stoi(key.str()), vote));
    }

    auto tss_sig_blob = map["tss_sig"].AsBlob();
    std::vector<uint8_t> tss_sig_bv(tss_sig_blob.data(), tss_sig_blob.data() + tss_sig_blob.size());
    auto tss_sig = flexbuffers_adapter<BLSSignature>::from_bytes(std::make_shared<std::vector<uint8_t>>(tss_sig_bv));

    auto block = std::make_shared<Block>();
    block->header_ = *bh;
    block->tx_vec_ = tx_vec;
    block->votes_ = votes;
    block->tss_sig_ = tss_sig;

    return block; 
}

}