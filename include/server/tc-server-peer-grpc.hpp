#include "spdlog/spdlog.h"
#include <easy/profiler.h>

#include <grpcpp/grpcpp.h>
#include "tc-server-peer.grpc.pb.h"

#include "tc-server.hpp"

namespace tomchain
{

    class TcPeerConsensusImpl final : public TcPeerConsensus::CallbackService
    {

    public:
        std::shared_ptr<TcServer> tc_server_;

    public:
        /**
         * @brief Server heartbeats with peers.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *SPHeartbeat(
            grpc::CallbackServerContext *context,
            const SPHeartbeatRequest *request,
            SPHeartbeatResponse *response) override
        {
            spdlog::debug("gRPC(SPHeartbeat) starts");

            uint32_t peer_id = request->id();

            response->set_status(0);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

        /**
         * @brief Server relays votes to its peer.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *RelayVote(
            grpc::CallbackServerContext *context,
            const RelayVoteRequest *request,
            RelayVoteResponse *response) override
        {
            EASY_BLOCK("RelayVoteRsp");
            spdlog::debug("gRPC(RelayVote) starts");

            uint32_t peer_id = request->id();
            auto req_votes = request->votes();

            // for (auto iter = req_votes.begin(); iter != req_votes.end(); iter++)
            for (size_t rv_index = 0; rv_index < req_votes.size(); rv_index++)
            {
                spdlog::trace("{} RelayVote: get relayed vote", peer_id);
                auto rv = req_votes.Get(rv_index);

                // deserialize relayed votes
                EASY_BLOCK("deserialize");
                spdlog::trace("{} RelayVote: deserialize relayed votes", peer_id);
                // msgpack::sbuffer des_b = stringToSbuffer(rv);
                // auto oh = msgpack::unpack(des_b.data(), des_b.size());
                // auto vote = oh->as<std::shared_ptr<BlockVote>>();
                std::vector<uint8_t> blkvote_ser(rv.begin(), rv.end());
                auto vote =
                    flexbuffers_adapter<BlockVote>::from_bytes(
                        std::make_shared<std::vector<uint8_t>>(blkvote_ser));

                const uint64_t block_id = vote->block_id_;
                EASY_END_BLOCK;

                // add to local block vote vector
                spdlog::trace("{} RelayVote: add to local block vote vector", peer_id);
                BlockCHM::accessor pb_accessor;

                EASY_BLOCK("find");
                spdlog::trace("{} RelayVote: finding block in pb", peer_id);
                bool is_found = tc_server_->pending_blks.find(pb_accessor, block_id);
                if (!is_found)
                {
                    spdlog::error("{} RelayVote: block ({}) not found", peer_id, block_id);
                    continue;
                }
                else
                {
                    spdlog::trace("{} RelayVote: block found", peer_id);
                }
                EASY_END_BLOCK;

                EASY_BLOCK("insert vote");
                std::shared_ptr<tomchain::Block> block_sp = pb_accessor->second;
                assert(block_sp != nullptr);
                block_sp->votes_.insert(
                    std::make_pair(
                        vote->voter_id_,
                        vote));
                EASY_END_BLOCK;

                // check if vote enough
                EASY_BLOCK("check vote enough");
                spdlog::trace("{} RelayVote: check if vote enough", peer_id);
                if (block_sp->is_vote_enough((*::conf_data)["client-count"]))
                {
                    spdlog::trace("{} RelayVote: vote enough", peer_id);

                    EASY_BLOCK("merge");
                    block_sp->merge_votes((*::conf_data)["client-count"]);
                    EASY_END_BLOCK;

                    // insert block to committed
                    EASY_BLOCK("insert cb");
                    spdlog::trace("{} RelayVote: insert block to committed", peer_id);
                    BlockCHM::accessor cb_accessor;
                    tc_server_->committed_blks.insert(
                        cb_accessor,
                        block_id);
                    cb_accessor->second = block_sp;
                    EASY_END_BLOCK;

                    pb_accessor.release();
                    spdlog::trace("{} RelayVote: pb_accessor released", peer_id);

                    // insert block to bcast commit
                    EASY_BLOCK("insert bcast commit");
                    spdlog::trace("{} RelayVote: insert block to bcast commit", peer_id);
                    for (
                        auto bcast_iter = tc_server_->bcast_commit_blocks.begin();
                        bcast_iter != tc_server_->bcast_commit_blocks.end();
                        bcast_iter++)
                    {
                        if (cb_accessor->second == nullptr)
                        {
                            spdlog::error("stub1");
                            exit(1);
                        }
                        if (bcast_iter->second == nullptr)
                        {
                            spdlog::error("stub2");
                            exit(1);
                        }
                        bcast_iter->second->push(block_sp);
                    }
                    EASY_END_BLOCK;
                    spdlog::trace("{} RelayVote: bcast commits", peer_id);

                    EASY_BLOCK("bcast commits");
                    tc_server_->bcast_commits();
                    EASY_END_BLOCK;

                    // remove block from pending
                    EASY_BLOCK("remove from pb");
                    spdlog::trace("{} RelayVote: remove block from pending", peer_id);
                    bool is_erased = tc_server_->pending_blks.erase(block_id);
                    if (is_erased)
                    {
                        spdlog::trace("{} RelayVote: block ({}) erased", peer_id, block_id);
                    }
                    else
                    {
                        spdlog::error("{} RelayVote: block ({}) not erased", peer_id, block_id);
                    }
                    EASY_END_BLOCK;

                    cb_accessor.release();
                }
                EASY_END_BLOCK;

                spdlog::trace("{} RelayVote: vote proc finished", peer_id);

                pb_accessor.release();
                spdlog::trace("{} RelayVote: pb_accessor released", peer_id);
            }

            response->set_status(0);

            spdlog::trace("{} RelayVote: ends proc", peer_id);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            spdlog::trace("{} RelayVote: ends", peer_id);

            EASY_END_BLOCK;

            return reactor;
        }

        /**
         * @brief Server relays new pending blocks to its peer.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *RelayBlock(
            grpc::CallbackServerContext *context,
            const RelayBlockRequest *request,
            RelayBlockResponse *response) override
        {
            EASY_BLOCK("RelayBlockResp");
            spdlog::debug("gRPC(RelayBlock) starts");

            uint32_t peer_id = request->id();
            auto req_blocks = request->blocks();

            for (auto iter = req_blocks.begin(); iter != req_blocks.end(); iter++)
            {
                // deserialize relayed blocks
                EASY_BLOCK("deserialize");
                spdlog::trace("{} RelayBlock: deserialize relayed blocks");
                msgpack::sbuffer des_b = stringToSbuffer(*iter);
                auto oh = msgpack::unpack(des_b.data(), des_b.size());
                auto block = oh->as<std::shared_ptr<Block>>();
                EASY_END_BLOCK;

                // store block locally
                EASY_BLOCK("store");
                spdlog::trace("{} RelayBlock: store block ({}) locally", peer_id, block->header_.id_);
                BlockCHM::accessor accessor;
                tc_server_->pending_blks.insert(accessor, block->header_.id_);
                accessor->second = block;
                accessor.release();
                EASY_END_BLOCK;
            }

            response->set_status(0);

            spdlog::trace("{} RelayBlock: ends proc", peer_id);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            spdlog::trace("{} RelayBlock: ends", peer_id);

            EASY_END_BLOCK;

            return reactor;
        }

        /**
         * @brief Server broadcast commit to its peer.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *SPBcastCommit(
            grpc::CallbackServerContext *context,
            const SPBcastCommitRequest *request,
            SPBcastCommitResponse *response) override
        {
            EASY_BLOCK("SPBcastCommit");
            spdlog::debug("gRPC(SPBcastCommit) starts");

            uint32_t peer_id = request->id();
            auto req_blocks = request->blocks();
            spdlog::trace("SPBcastCommit: req_blocks size: {}", req_blocks.size());

            for (auto iter = req_blocks.begin(); iter != req_blocks.end(); iter++)
            {
                // deserialize bcasted blocks
                EASY_BLOCK("deserialize");
                spdlog::trace("SPBcastCommit: deserialize bcasted blocks");
                msgpack::sbuffer des_b = stringToSbuffer(*iter);
                auto oh = msgpack::unpack(des_b.data(), des_b.size());
                auto block = oh->as<std::shared_ptr<Block>>();
                EASY_END_BLOCK;

                // remove pending block
                EASY_BLOCK("remove pb");
                spdlog::trace("SPBcastCommit: remove pending block");
                BlockCHM::accessor pb_accessor;
                bool is_found = tc_server_->pending_blks.find(
                    pb_accessor, block->header_.id_);
                if (!is_found)
                {
                    spdlog::error("SPBcastCommit: block not found");
                }
                EASY_END_BLOCK;

                // insert into committed blocks
                EASY_BLOCK("insert cb");
                spdlog::trace("insert into committed blocks");
                BlockCHM::accessor cb_accessor;
                tc_server_->committed_blks.insert(
                    cb_accessor,
                    block->header_.id_);
                cb_accessor->second = block;
                EASY_END_BLOCK;

                EASY_BLOCK("erase");
                tc_server_->pending_blks.erase(pb_accessor);
                EASY_END_BLOCK;

                cb_accessor.release();
                pb_accessor.release();
            }

            response->set_status(0);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            EASY_END_BLOCK;

            return reactor;
        }

        /**
         * @brief Server sends relayed block sync signal to another.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *RelayBlockSync(
            grpc::CallbackServerContext *context,
            const RelayBlockSyncRequest *request,
            RelayBlockSyncResponse *response) override
        {
            EASY_BLOCK("RelayBlockSync");
            spdlog::debug("gRPC(RelayBlockSync) starts");

            uint32_t peer_id = request->id();
            uint64_t block_id = request->block_id();

            // insert sync label
            EASY_BLOCK("insert signal");
            tc_server_->pb_sync_labels.insert(block_id);
            spdlog::trace("{} RelayBlockSync: block ({}) signaled", peer_id, block_id);
            EASY_END_BLOCK;

            response->set_status(0);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            EASY_END_BLOCK;

            return reactor;
        }
    };

    grpc::Status TcServer::SPHeartbeat(uint64_t target_server_id)
    {
        SPHeartbeatRequest request;
        request.set_id(this->server_id);

        SPHeartbeatResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        grpc::Status status;
        grpc_peer_client_stub_.find(target_server_id)->second->async()->SPHeartbeat(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s)
                                                                                    {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one(); });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }

        spdlog::trace("gRPC(SPHeartbeat): {}:{}",
                      status.error_code(),
                      status.error_message());

        return status;
    }

    grpc::Status TcServer::RelayVote(uint64_t target_server_id)
    {
        EASY_BLOCK("RelayVoteReq");
        spdlog::trace("{} gRPC(RelayVote) starts", target_server_id);

        RelayVoteRequest request;
        request.set_id(this->server_id);

        EASY_BLOCK("add votes");
        std::shared_ptr<BlockVote> vote;
        spdlog::trace("{} gRPC(RelayVote) pop votes", target_server_id);
        while (relay_votes.find(target_server_id)->second->try_pop(vote))
        {
            // serialize vote
            // msgpack::sbuffer b;
            // msgpack::pack(b, vote);
            // std::string ser_vote = sbufferToString(b);
            auto blk_bv = flexbuffers_adapter<BlockVote>::to_bytes(*vote);
            std::string ser_vote(blk_bv->begin(), blk_bv->end());

            // add to relayed vote vector
            request.add_votes(ser_vote);
        }
        EASY_END_BLOCK;

        // if no votes, return
        if (request.votes_size() == 0)
        {
            return grpc::Status::OK;
        }

        RelayVoteResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("waiting");
        spdlog::trace("{} gRPC(RelayVote) waiting", target_server_id);
        grpc::Status status;
        grpc_peer_client_stub_.find(target_server_id)->second->async()->RelayVote(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s)
                                                                                  {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one(); });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        spdlog::trace("gRPC(RelayVote): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

    grpc::Status TcServer::RelayBlock(uint64_t target_server_id)
    {
        EASY_BLOCK("RelayBlockReq");
        spdlog::trace("{} gRPC(RelayBlock) starts", target_server_id);

        RelayBlockRequest request;
        request.set_id(this->server_id);

        std::vector<uint64_t> tmp_sync_vec;

        std::shared_ptr<Block> block;
        EASY_BLOCK("add blocks");
        spdlog::trace("{} gRPC(RelayBlock) pops blocks", target_server_id);
        while (relay_blocks.find(target_server_id)->second->try_pop(block))
        {
            // serialize block
            EASY_BLOCK("serialize");
            msgpack::sbuffer b;
            msgpack::pack(b, block);
            std::string ser_block = sbufferToString(b);
            EASY_END_BLOCK;

            // add to relayed block vector
            EASY_BLOCK("add to relay");
            request.add_blocks(ser_block);
            tmp_sync_vec.push_back(block->header_.id_);
            EASY_END_BLOCK;
        }
        EASY_END_BLOCK;

        // if no blocks, return
        if (request.blocks_size() == 0)
        {
            return grpc::Status::OK;
        }

        RelayBlockResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("wait");
        spdlog::trace("{} gRPC(RelayBlock) waiting", target_server_id);
        grpc::Status status;
        grpc_peer_client_stub_.find(target_server_id)->second->async()->RelayBlock(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s)
                                                                                   {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one(); });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        // add block ids to sync queue
        EASY_BLOCK("add sync queue");
        for (auto id : tmp_sync_vec)
        {
            this->pb_sync_queue.push(id);
        }
        EASY_END_BLOCK;

        spdlog::trace("gRPC(RelayBlock): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

    grpc::Status TcServer::SPBcastCommit(uint64_t target_server_id)
    {
        EASY_BLOCK("SPBcastCommitReq");
        spdlog::trace("{} gRPC(SPBcastCommit) starts", target_server_id);

        SPBcastCommitRequest request;
        request.set_id(this->server_id);

        std::shared_ptr<Block> block;
        spdlog::trace("{} gRPC(SPBcastCommit) pops blocks", target_server_id);
        while (bcast_commit_blocks.find(target_server_id)->second->try_pop(block))
        {
            if (block == nullptr)
            {
                spdlog::error("block is nullptr");
                exit(1);
            }

            try
            {
                // serialize block
                EASY_BLOCK("serialize");
                msgpack::sbuffer b;
                msgpack::pack(b, block);
                std::string ser_block = sbufferToString(b);
                EASY_END_BLOCK;

                // add to bcast block vector
                request.add_blocks(ser_block);
            }
            catch (const std::exception &e)
            {
                spdlog::error("SPBcastCommit: {}", e.what());
                exit(1);
            }
        }

        // if no commits, return
        if (request.blocks_size() == 0)
        {
            return grpc::Status::OK;
        }

        SPBcastCommitResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("waiting");
        spdlog::trace("{} gRPC(SPBcastCommit) waiting", target_server_id);
        grpc::Status status;
        grpc_peer_client_stub_.find(target_server_id)->second->async()->SPBcastCommit(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s)
                                                                                      {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one(); });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        spdlog::trace("gRPC(SPBcastCommit): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

    grpc::Status TcServer::RelayBlockSync(uint64_t block_id, uint64_t target_server_id)
    {
        EASY_BLOCK("RelayBlockSyncReq");
        spdlog::trace("{} gRPC(RelayBlockSync) starts", target_server_id);

        RelayBlockSyncRequest request;
        request.set_id(this->server_id);
        request.set_block_id(block_id);

        RelayBlockSyncResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("waiting");
        spdlog::trace("{} gRPC(RelayBlockSync) waiting", target_server_id);
        grpc::Status status;
        grpc_peer_client_stub_.find(target_server_id)->second->async()->RelayBlockSync(&context, &request, &response, [&mu, &cv, &done, &status](grpc::Status s)
                                                                                       {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one(); });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        spdlog::trace("gRPC(RelayBlockSync): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

}
