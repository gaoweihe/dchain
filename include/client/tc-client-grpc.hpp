#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "flexbuffers_adapter.hpp"

namespace tomchain {

grpc::Status TcClient::Register()
{
    RegisterRequest request; 
    request.set_id(this->client_id);
    const std::vector<uint8_t>& ser_pkey = this->ecc_pkey->get_pub_key_data();
    request.set_pkey(ser_pkey.data(), ser_pkey.size());
    RegisterResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->Register(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    auto tss_sk_str = response.tss_sk();
    BLSPrivateKeyShare skey_share(
        tss_sk_str, 
        (*::conf_data)["client-count"], 
        (*::conf_data)["client-count"]
    );
    std::shared_ptr<libff::alt_bn128_Fr> skey_raw = skey_share.getPrivateKey(); 
    BLSPublicKeyShare pkey_share(
        *skey_raw, 
        (*::conf_data)["client-count"], 
        (*::conf_data)["client-count"]
    ); 
    this->tss_key = std::make_shared<std::pair<
        std::shared_ptr<BLSPrivateKeyShare>, 
        std::shared_ptr<BLSPublicKeyShare>
    >>(
        std::make_pair<
            std::shared_ptr<BLSPrivateKeyShare>, 
            std::shared_ptr<BLSPublicKeyShare>
        >(
            std::make_shared<BLSPrivateKeyShare>(skey_share), 
            std::make_shared<BLSPublicKeyShare>(pkey_share)
        )
    );

    spdlog::debug("gRPC(Register): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::Heartbeat()
{
    HeartbeatRequest request; 
    request.set_id(this->client_id); 
    HeartbeatResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->Heartbeat(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    spdlog::debug("gRPC(Heartbeat): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::PullPendingBlocks()
{
    EASY_FUNCTION("PullPendingBlocks_req");
    spdlog::trace("gRPC(PullPendingBlocks) starts");

    PullPendingBlocksRequest request; 
    request.set_id(this->client_id); 
    PullPendingBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    EASY_BLOCK("waiting");
    grpc::Status status;
    stub_->async()->PullPendingBlocks(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }
    EASY_END_BLOCK; 

    // unpack response 
    EASY_BLOCK("unpack response");
    for (int i = 0; i < response.pb_hdrs_size(); i++) {
        msgpack::sbuffer des_b = stringToSbuffer(response.pb_hdrs(i));
        auto oh = msgpack::unpack(des_b.data(), des_b.size());
        auto block_hdr = oh->as<BlockHeader>();

        pending_blkhdr.insert(
            std::make_pair(
                block_hdr.id_, 
                std::make_shared<BlockHeader>(block_hdr)
            )
        );
        spdlog::trace("block id: {}", block_hdr.id_); 
    }
    EASY_END_BLOCK; 

    if (response.pb_hdrs_size() == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    spdlog::debug("gRPC(PullPendingBlocks): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::GetBlocks()
{
    EASY_FUNCTION("GetBlocks_req");
    spdlog::debug("gRPC(GetBlocks): start");

    GetBlocksRequest request; 

    EASY_BLOCK("render request");
    for (auto iter = pending_blkhdr.begin(); iter != pending_blkhdr.end(); iter++)
    {
        msgpack::sbuffer b;
        msgpack::pack(b, iter->second); 
        std::string blk_hdr_str = sbufferToString(b);

        request.add_pb_hdrs(blk_hdr_str);
    }
    EASY_END_BLOCK; 
    
    GetBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    EASY_BLOCK("waiting");
    grpc::Status status;
    stub_->async()->GetBlocks(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }
    EASY_END_BLOCK; 

    EASY_BLOCK("unpack response");
    auto resp_blk = response.pb();
    for (auto iter = resp_blk.begin(); iter != resp_blk.end(); iter++)
    {
        EASY_BLOCK("deserialize");
        msgpack::sbuffer des_b = stringToSbuffer(*iter);
        auto oh = msgpack::unpack(des_b.data(), des_b.size());
        auto block = oh->as<Block>();
        EASY_END_BLOCK; 

        EASY_BLOCK("insert into pb");
        pending_blks.push(
            std::make_shared<tomchain::Block>(block)
        ); 
        EASY_END_BLOCK; 

        // EASY_BLOCK("vote");
        // this->VoteBlocks(); 
        // EASY_END_BLOCK; 

        // remove block header from CHM
        EASY_BLOCK("remove");
        pending_blkhdr.erase(block.header_.id_); 
        EASY_END_BLOCK; 

        spdlog::trace("get block: {}, {}", block.header_.id_, block.header_.base_id_); 
    }
    EASY_END_BLOCK; 
    

    spdlog::debug("gRPC(GetBlocks): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status; 
}

grpc::Status TcClient::VoteBlocks()
{
    EASY_FUNCTION("VoteBlocks_req");
    spdlog::debug("gRPC(VoteBlocks): start");

    VoteBlocksRequest request; 
    request.set_id(this->client_id); 
    
    std::shared_ptr<Block> sp_block; 
    while (pending_blks.try_pop(sp_block))
    // for (auto iter = pending_blks.begin(); iter != pending_blks.end(); iter++)
    {
        auto block_hash_str = sp_block->get_sha256();

        const uint64_t block_id = sp_block->header_.id_; 

        // client_id starts from 1, so does signer_index 
        EASY_BLOCK("sign");
        auto signer_index = this->client_id;
        std::shared_ptr<BLSSigShare> sig_share = 
            this->tss_key->first->sign(block_hash_str, this->client_id); 
        BlockVote bv;
        bv.block_id_ = sp_block->header_.id_;
        bv.sig_share_ = sig_share;
        bv.voter_id_ = this->client_id;
        EASY_END_BLOCK; 

        EASY_BLOCK("insert into votes");
        sp_block->votes_.insert(
            std::make_pair(
                this->client_id, 
                std::make_shared<BlockVote>(bv)
            )
        );
        EASY_END_BLOCK; 

        EASY_BLOCK("serialize");
        spdlog::debug("gRPC(VoteBlocks): serialize");
        Block block = *(sp_block);
        // msgpack::sbuffer b;
        // msgpack::pack(b, iter->second); 
        // std::string block_ser = sbufferToString(b);
        auto block_bv = flexbuffers_adapter<Block>::to_bytes(block);
        std::string block_ser(block_bv->begin(), block_bv->end());
        EASY_END_BLOCK; 
      
        EASY_BLOCK("add voted blocks");
        request.add_voted_blocks(block_ser); 
        EASY_END_BLOCK; 
    }
    
    VoteBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    EASY_BLOCK("waiting");
    spdlog::debug("gRPC(VoteBlocks): send request");
    grpc::Status status;
    stub_->async()->VoteBlocks(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }
    EASY_END_BLOCK; 

    EASY_BLOCK("unpack response");
    spdlog::debug("gRPC(VoteBlocks): recv response");
    // for (auto iter = pending_blks.begin(); iter != pending_blks.end(); iter++)
    // {
    //     voted_blks.insert(
    //         std::make_pair(
    //             iter->first, 
    //             iter->second
    //         )
    //     ); 
    // }
    // pending_blks.clear(); 
    EASY_END_BLOCK; 
    

    spdlog::debug("gRPC(VoteBlocks): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status; 
}

}; 
