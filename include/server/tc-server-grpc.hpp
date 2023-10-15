#include <memory>
#include <chrono>

#include "spdlog/spdlog.h"
#include <easy/profiler.h>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "flexbuffers_adapter.hpp"

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
            spdlog::trace("gRPC(Register) starts");

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
            spdlog::trace("gRPC(Heartbeat) starts");

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
            EASY_BLOCK("PullPendingBlocksResp");
            spdlog::trace("gRPC(PullPendingBlocks) starts");

            uint64_t client_id = request->id();

            response->set_status(0);

            BlockCHM::const_accessor accessor;
            ClientCHM::const_accessor client_accessor;
            BlockHeaderCHM::accessor seenblk_accessor;

            // unsafe iterations on concurrent hash map
            std::unique_lock<std::shared_mutex> pb_ul_1(tc_server_->pb_sm_1);
            BlockCHM shadow_pb(tc_server_->pending_blks); 
            pb_ul_1.unlock(); 
            for (auto iter = shadow_pb.begin(); iter != shadow_pb.end(); iter++)
            {
                bool is_found = false;
                try
                {
                    EASY_BLOCK("find block");
                    is_found = shadow_pb.find(accessor, iter->first);
                    EASY_END_BLOCK;

                    if (is_found)
                    {
                        // check if got sync signal
                        bool is_synced = tc_server_->pb_sync_labels.contains(iter->first);
                        if (!is_synced)
                        {
                            accessor.release();
                            continue;
                        }

                        std::shared_ptr<Block> blk = iter->second;

                        // TODO: check client seen blocks
                        tc_server_->clients.find(client_accessor, client_id);
                        bool is_bh_found = client_accessor->second->seen_blocks.find(seenblk_accessor, blk->header_.id_);
                        if (is_bh_found)
                        {
                            seenblk_accessor.release();
                        }
                        else
                        {
                            // record client seen blocks
                            client_accessor->second->seen_blocks.insert(
                                std::make_pair(
                                    blk->header_.id_,
                                    std::make_shared<BlockHeader>(blk->header_)));

                            EASY_BLOCK("serialize response");
                            // msgpack::sbuffer b;
                            // msgpack::pack(b, blk->header_);
                            // std::string blk_hdr_str = sbufferToString(b);
                            auto blk_bv = flexbuffers_adapter<BlockHeader>::to_bytes(blk->header_);
                            std::string blk_hdr_str(blk_bv->begin(), blk_bv->end());
                            EASY_END_BLOCK;

                            // record distribution timestamp 
                            const uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count(); 
                            // spdlog::trace("now_ms: {}", now_ms); 
                            if (blk->header_.dist_ts_ == 0)
                            {
                                blk->header_.dist_ts_ = now_ms; 
                            }

                            EASY_BLOCK("add header");
                            response->add_pb_hdrs(blk_hdr_str);
                            EASY_END_BLOCK;

                            seenblk_accessor.release();
                            client_accessor.release();
                        }
                    }

                    accessor.release();
                }
                catch (std::exception &e)
                {
                    seenblk_accessor.release();
                    client_accessor.release();
                    accessor.release();
                    continue;
                }
            }

            spdlog::trace("gRPC(PullPendingBlocks) ends");

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            EASY_END_BLOCK;

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
            EASY_BLOCK("GetBlocksResp");
            spdlog::trace("gRPC(GetBlocks) starts");

            response->set_status(0);

            auto req_blk_hdr = request->pb_hdrs();
            BlockCHM::const_accessor accessor;

            // unsafe interations on concurrent hash map
            // but it is serial
            for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
            {
                // deserialize requested block headers
                EASY_BLOCK("deserialize request");
                spdlog::trace("deserialize requested block headers");
                // msgpack::sbuffer des_b = stringToSbuffer(*iter);
                // auto oh = msgpack::unpack(des_b.data(), des_b.size());
                // auto blk_hdr = oh->as<BlockHeader>();
                std::vector<uint8_t> blkhdr_ser((*iter).begin(), (*iter).end());
                auto blk_hdr =
                    flexbuffers_adapter<BlockHeader>::from_bytes(
                        std::make_shared<std::vector<uint8_t>>(blkhdr_ser));
                EASY_END_BLOCK;

                // find local blocks
                EASY_BLOCK("find local block");
                spdlog::trace("find local block");
                std::shared_lock<std::shared_mutex> pb_sl_1(tc_server_->pb_sm_1);
                bool is_found = tc_server_->pending_blks.find(accessor, blk_hdr->id_);
                pb_sl_1.unlock();
                if (!is_found)
                {
                    spdlog::trace("block not found");
                    continue;
                }
                EASY_END_BLOCK;

                std::shared_ptr<Block> block = accessor->second;
                spdlog::trace("pb tx count={}", block->tx_vec_.size()); 

                // serialize block
                EASY_BLOCK("serialize response");
                spdlog::trace("serialize block");
                // msgpack::sbuffer b;
                // msgpack::pack(b, block);
                // std::string ser_blk = sbufferToString(b);
                auto blk_bv = flexbuffers_adapter<Block>::to_bytes(*block);
                std::string ser_blk(blk_bv->begin(), blk_bv->end());
                EASY_END_BLOCK;

                // add serialized block to response
                EASY_BLOCK("add block");
                spdlog::trace("add serialized block to response");
                response->add_pb(ser_blk);
                EASY_END_BLOCK;
            }

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            EASY_END_BLOCK;

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
            EASY_BLOCK("VoteBlocksResp");
            spdlog::trace("gRPC(VoteBlocks) starts");

            response->set_status(0);

            auto client_id = request->id();
            if (client_id == 1)
            {
                spdlog::debug("received client 1"); 
            }
            auto voted_blocks = request->voted_blocks();
            spdlog::trace("{}:voted blocks count={}",
                          client_id,
                          voted_blocks.size());

            EASY_BLOCK("traverse");
            for (auto iter = voted_blocks.begin(); iter != voted_blocks.end(); iter++)
            {
                // deserialize request
                EASY_BLOCK("deserialize request");
                spdlog::trace("{}:deserialize request", client_id);
                // msgpack::sbuffer des_b = stringToSbuffer(*iter);
                // auto oh = msgpack::unpack(des_b.data(), des_b.size());
                // auto block = oh->as<std::shared_ptr<Block>>();
                std::vector<uint8_t> block_ser((*iter).begin(), (*iter).end());
                auto block =
                    flexbuffers_adapter<Block>::from_bytes(
                        std::make_shared<std::vector<uint8_t>>(block_ser));
                EASY_END_BLOCK;

                // get block vote from request
                EASY_BLOCK("get block vote from request");
                spdlog::trace("{}:get block vote from request", client_id);
                auto vote = block->votes_.find(request->id());
                if (vote == block->votes_.end())
                {
                    spdlog::trace("{}:vote not found", client_id);
                    continue;
                }
                EASY_END_BLOCK;

                // check if dead block 
                EASY_BLOCK("check if dead block");
                bool is_died = tc_server_->dead_block.contains(block->header_.id_);
                if (is_died)
                {
                    spdlog::trace("{}:block is dead", client_id);
                    continue;
                }
                EASY_END_BLOCK; 

                // check if target server is this server
                EASY_BLOCK("calculate target server id");
                std::set<uint64_t> target_server_id_set = block->get_server_id((*::conf_data)["server-count"]);
                // if this server is not BPS 
                if (target_server_id_set.find(tc_server_->server_id) == target_server_id_set.end())
                {
                    // if remote
                    // insert vote into relay queue and skip this iteration
                    for (auto iter = target_server_id_set.begin(); iter != target_server_id_set.end(); iter++)
                    {
                        tc_server_->relay_votes.find(*iter)->second->push(vote->second);
                    }
                    // tc_server_->send_relay_votes();
                    continue;
                }
                else // relay to peer shadow server 
                {
                    for (auto iter = target_server_id_set.begin(); iter != target_server_id_set.end(); iter++)
                    {
                        if (*iter == tc_server_->server_id)
                        {
                            continue; 
                        }
                        tc_server_->relay_votes.find(*iter)->second->push(vote->second);
                    }
                }
                EASY_END_BLOCK;

                // find local block storage
                EASY_BLOCK("find local block storage");
                spdlog::trace("{}:find local block storage", client_id);
                BlockCHM::accessor pb_accessor;
                std::shared_lock<std::shared_mutex> pb_sl_1(tc_server_->pb_sm_1);
                bool block_is_found = tc_server_->pending_blks.find(pb_accessor, block->header_.id_);
                pb_sl_1.unlock();
                if (!block_is_found)
                {
                    spdlog::trace("{}:block not found", client_id);
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
                spdlog::debug("{}:push vote into {} relay queue, vote count={}", 
                    client_id, 
                    block->header_.id_, 
                    pb_accessor->second->votes_.size()
                );
                EASY_END_BLOCK;

                // // TODO: if peer is not down and current server is not BPS, continue 
                // const uint64_t peer_shadow_server_id = tc_server_->get_shadow_peer_server_id(); 
                // const uint64_t peer_shadow_server_index = peer_shadow_server_id - 1; 
                // if (tc_server_->peer_status.at(peer_shadow_server_index).load() == true && 
                //     target_server_id_set.find(tc_server_->server_id) == target_server_id_set.end())
                // {
                //     pb_accessor.release();
                //     continue; 
                // }

                // if votes count enough
                EASY_BLOCK("count votes");
                spdlog::trace("{}:check if votes count enough", client_id);
                if (pb_accessor->second->is_vote_enough((*::conf_data)["client-count"]))
                {
                    // pb_accessor.release();
                    spdlog::debug("push into pb_merge_queue"); 
                    tc_server_->pb_merge_queue.push(pb_accessor->second);
                    // pb_accessor.release(); 
                    std::shared_lock<std::shared_mutex> pb_sl_1(tc_server_->pb_sm_1);
                    tc_server_->pending_blks.erase(pb_accessor); 
                    pb_sl_1.unlock();
                }

                pb_accessor.release();

                EASY_END_BLOCK;
            }
            EASY_END_BLOCK; 

            grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish(grpc::Status::OK);

            EASY_END_BLOCK;

            return reactor;
        }

    public:
        std::shared_ptr<TcConsensusImpl>
            consensus_;
    };
}
