#include "spdlog/spdlog.h"
#include <easy/profiler.h>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include <memory>

namespace tomchain
{
    class TcServer; 

    class TcConsensusImpl final : public TcConsensus::CallbackService
    {

    public:
        /**
         * @brief Reference to TomChain server instance
         *
         */
        std::shared_ptr<TcServer> tc_server_;

    public:
        /**
         * @brief Client registers when it connects to server.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *Register(
            grpc::CallbackServerContext *context,
            const RegisterRequest *request,
            RegisterResponse *response) override
        {
            spdlog::debug("gRPC(Register) starts");

            uint32_t client_id = request->id();
            std::string pkey_str = request->pkey();
            std::vector<uint8_t> pkey_data_vec(pkey_str.begin(), pkey_str.end());
            ecdsa::PubKey pkey(pkey_data_vec);

            // lock weak pointer to get shared pointer
            std::shared_ptr<TcServer> shared_tc_server = tc_server_;

            // update ecdsa pubic key
            ClientCHM::accessor accessor;
            shared_tc_server->clients.find(accessor, client_id);
            accessor->second->ecc_pkey = std::make_shared<ecdsa::PubKey>(
                std::move(pkey));

            response->set_id(client_id);
            response->set_tss_sk(*(accessor->second->tss_key->first->toString()));
            response->set_status(0);

            accessor.release();

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

        /**
         * @brief Client heartbeats by a given interval.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *Heartbeat(
            grpc::CallbackServerContext *context,
            const HeartbeatRequest *request,
            HeartbeatResponse *response) override
        {
            spdlog::debug("gRPC(Heartbeat) starts");

            response->set_status(0);

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

        /**
         * @brief Client pulls pending blocks.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *PullPendingBlocks(
            grpc::CallbackServerContext *context,
            const PullPendingBlocksRequest *request,
            PullPendingBlocksResponse *response) override
        {
            EASY_FUNCTION("PullPendingBlocks");
            spdlog::debug("gRPC(PullPendingBlocks) starts");

            response->set_status(0);

            BlockCHM::accessor accessor;

            // unsafe iterations on concurrent hash map
            for (auto iter = tc_server_->pending_blks.begin(); iter != tc_server_->pending_blks.end(); iter++)
            {
                bool is_found = false;
                try
                {
                    is_found = tc_server_->pending_blks.find(accessor, iter->first);

                    if (is_found)
                    {
                        std::shared_ptr<Block> blk = iter->second;

                        EASY_BLOCK("serialize response");
                        msgpack::sbuffer b;
                        msgpack::pack(b, blk->header_);
                        std::string blk_hdr_str = sbufferToString(b);
                        EASY_END_BLOCK; 

                        response->add_pb_hdrs(blk_hdr_str);
                    }
                }
                catch (std::exception &e)
                {
                    accessor.release();
                    continue;
                }
            }

            spdlog::debug("gRPC(PullPendingBlocks) ends");

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

        /**
         * @brief Client gets blocks.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *GetBlocks(
            grpc::CallbackServerContext *context,
            const GetBlocksRequest *request,
            GetBlocksResponse *response) override
        {
            EASY_FUNCTION("GetBlocks");
            spdlog::debug("gRPC(GetBlocks) starts");

            response->set_status(0);

            auto req_blk_hdr = request->pb_hdrs();
            BlockCHM::accessor accessor;

            // unsafe interations on concurrent hash map
            // but it is serial
            for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
            {
                // deserialize requested block headers
                EASY_BLOCK("deserialize request");
                spdlog::trace("deserialize requested block headers");
                msgpack::sbuffer des_b = stringToSbuffer(*iter);
                auto oh = msgpack::unpack(des_b.data(), des_b.size());
                auto blk_hdr = oh->as<BlockHeader>();
                EASY_END_BLOCK; 

                // find local blocks
                spdlog::trace("find local blocks");
                bool is_found = tc_server_->pending_blks.find(accessor, blk_hdr.id_);
                if (!is_found)
                {
                    spdlog::error("block not found");
                    continue;
                }
                std::shared_ptr<Block> block = accessor->second;

                // serialize block
                EASY_BLOCK("serialize response");
                spdlog::trace("serialize block");
                msgpack::sbuffer b;
                msgpack::pack(b, block);
                std::string ser_blk = sbufferToString(b);
                EASY_END_BLOCK; 

                // add serialized block to response
                spdlog::trace("add serialized block to response");
                response->add_pb(ser_blk);
            }

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

        /**
         * @brief Client votes blocks.
         *
         * @param context RPC context.
         * @param request RPC request.
         * @param response RPC response.
         * @return grpc::Status RPC status.
         */
        grpc::ServerUnaryReactor *VoteBlocks(
            grpc::CallbackServerContext *context,
            const VoteBlocksRequest *request,
            VoteBlocksResponse *response) override
        {
            EASY_FUNCTION("VoteBlocks");
            spdlog::debug("gRPC(VoteBlocks) starts");

            response->set_status(0);

            auto client_id = request->id();
            auto voted_blocks = request->voted_blocks();
            spdlog::trace("{}:voted blocks count={}",
                          client_id,
                          voted_blocks.size());

            // unsafe interations on concurrent hash map
            // but it is serial
            for (auto iter = voted_blocks.begin(); iter != voted_blocks.end(); iter++)
            {
                // deserialize request
                EASY_BLOCK("deserialize request");
                spdlog::trace("{}:deserialize request", client_id);
                msgpack::sbuffer des_b = stringToSbuffer(*iter);
                auto oh = msgpack::unpack(des_b.data(), des_b.size());
                auto block = oh->as<std::shared_ptr<Block>>();
                EASY_END_BLOCK;

                // get block vote from request
                EASY_BLOCK("get block vote from request");
                spdlog::trace("{}:get block vote from request", client_id);
                auto vote = block->votes_.find(request->id());
                if (vote == block->votes_.end())
                {
                    spdlog::error("{}:vote not found", client_id);
                    continue;
                }
                EASY_END_BLOCK;

                // check if target server is this server
                EASY_BLOCK("calculate target server id");
                const uint64_t target_server_id = block->get_server_id((*::conf_data)["server-count"]);
                if (target_server_id != tc_server_->server_id)
                {
                    // if remote
                    // insert vote into relay queue and skip this iteration
                    tc_server_->relay_votes.find(target_server_id)->second->push(vote->second);
                    continue;
                }
                EASY_END_BLOCK;

                // find local block storage
                EASY_BLOCK("find local block storage");
                spdlog::trace("{}:find local block storage", client_id);
                BlockCHM::accessor pb_accessor;
                bool block_is_found = tc_server_->pending_blks.find(pb_accessor, block->header_.id_);
                if (!block_is_found)
                {
                    spdlog::error("{}:block not found", client_id);
                    continue;
                }
                EASY_END_BLOCK;

                // insert received vote
                EASY_BLOCK("insert received vote");
                spdlog::trace("{}:insert received vote", client_id);
                pb_accessor->second->votes_.insert(
                    std::make_pair(
                        request->id(),
                        vote->second));
                EASY_END_BLOCK;

                // if votes count enough
                EASY_BLOCK("count votes");
                spdlog::trace("{}:check if votes count enough", client_id);
                if (pb_accessor->second->is_vote_enough((*::conf_data)["client-count"]))
                {
                    pb_accessor->second->merge_votes((*::conf_data)["client-count"]);

                    // insert block to committed
                    BlockCHM::accessor cb_accessor;
                    tc_server_->committed_blks.insert(
                        cb_accessor,
                        block->header_.id_);
                    cb_accessor->second = pb_accessor->second;

                    // insert block to bcast commit
                    for (
                        auto iter = tc_server_->bcast_commit_blocks.begin(); 
                        iter != tc_server_->bcast_commit_blocks.end(); 
                        iter++
                    ) {
                        iter->second->push(cb_accessor->second);
                    }

                    // remove block from pending
                    spdlog::trace("{}:remove block from pending", client_id);
                    tc_server_->pending_blks.erase(pb_accessor);

                    cb_accessor.release();
                }

                pb_accessor.release();

                EASY_END_BLOCK;
            }

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);
            return reactor;
        }

    public:
        std::shared_ptr<TcConsensusImpl>
            consensus_;
    };
}
