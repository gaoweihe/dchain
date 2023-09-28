#include "flexbuffers_adapter.hpp"

#include "spdlog/spdlog.h"

#include <random>

using namespace tomchain;

int main()
{
    std::shared_ptr<BLSSignature> serial_sig;
    std::shared_ptr<BLSSignature> parallel_sig;

    std::array<uint8_t, 32> hash_arr;
    std::shared_ptr<std::array<uint8_t, 32>> spHashArr =
        std::make_shared<std::array<uint8_t, 32>>(hash_arr);

    const size_t num_signed = 100;
    const size_t num_all = 100;
    std::shared_ptr<std::pair<
        std::shared_ptr<std::vector<std::shared_ptr<BLSPrivateKeyShare>>>,
        std::shared_ptr<BLSPublicKey>>>
        keys =
            BLSPrivateKeyShare::generateSampleKeys(num_signed, num_all);

    std::shared_ptr<BLSPrivateKeyShare> skey_share = keys->first->at(0);
    std::shared_ptr<BLSSigShare> sig_share = skey_share->sign(spHashArr, 1);

    std::shared_ptr<std::vector<uint8_t>> bytes_ser = flexbuffers_adapter<BLSSigShare>::to_bytes(*sig_share);
    std::shared_ptr<BLSSigShare> sig_share_des = flexbuffers_adapter<BLSSigShare>::from_bytes(bytes_ser);

    spdlog::info("sig_share");
    assert(*(sig_share->getSigShare()) == *(sig_share_des->getSigShare()));

    Block block;

    BlockVote vote;
    vote.block_id_ = block.header_.id_;
    vote.voter_id_ = 1;
    vote.sig_share_ = sig_share;

    block.votes_.insert(
        std::make_pair(
            1,
            std::make_shared<BlockVote>(vote)));

    BLSSigShareSet sig_set(1, 1);
    sig_set.addSigShare(sig_share);
    std::shared_ptr<BLSSignature> tss_sig = sig_set.merge();

    BlockHeader header;
    header.base_id_ = 1;
    header.id_ = 1;
    block.header_ = header;

    block.tss_sig_ = tss_sig;

    std::shared_ptr<std::vector<uint8_t>> bytes = flexbuffers_adapter<Block>::to_bytes(block);
    std::shared_ptr<Block> block_des = flexbuffers_adapter<Block>::from_bytes(bytes);

    assert(block_des->header_.base_id_ == block.header_.base_id_);
    spdlog::info("block");

    // test block header
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> distribution(1, INT_MAX);
        uint64_t header_id = distribution(rng);
        uint64_t header_baseid = distribution(rng);
        BlockHeader hdr1(header_id, header_baseid, 1);
        auto hdr_bv = flexbuffers_adapter<BlockHeader>::to_bytes(hdr1);
        std::string hdr1_ser(hdr_bv->begin(), hdr_bv->end());

        std::vector<uint8_t> blkhdr_ser(hdr1_ser.begin(), hdr1_ser.end());
        auto hdr1_des =
            flexbuffers_adapter<BlockHeader>::from_bytes(
                std::make_shared<std::vector<uint8_t>>(blkhdr_ser));
        spdlog::info("hdr1: {} {}", header_id, header_baseid);
        spdlog::info("hdr1: {} {}", hdr1_des->id_, hdr1_des->base_id_);
    }

    return 0;
}