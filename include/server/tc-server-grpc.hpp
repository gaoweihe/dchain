class TcConsensusImpl final : 
    public TcConsensus::CallbackService {

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
    grpc::ServerUnaryReactor* Register(
        grpc::CallbackServerContext* context, 
        const RegisterRequest* request,
        RegisterResponse* response
    ) override
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
            std::move(pkey)
        ); 

        response->set_id(client_id);
        response->set_tss_sk(*(accessor->second->tss_key->first->toString()));
        response->set_status(0); 

        accessor.release(); 

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
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
    grpc::ServerUnaryReactor* Heartbeat(
        grpc::CallbackServerContext* context, 
        const HeartbeatRequest* request,
        HeartbeatResponse* response
    ) override
    {
        spdlog::debug("gRPC(Heartbeat) starts");

        response->set_status(0); 

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
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
    grpc::ServerUnaryReactor* PullPendingBlocks(
        grpc::CallbackServerContext* context, 
        const PullPendingBlocksRequest* request,
        PullPendingBlocksResponse* response
    ) override
    {
        spdlog::debug("gRPC(PullPendingBlocks) starts"); 

        response->set_status(0);  

        BlockCHM::accessor accessor;

        // unsafe iterations on concurrent hash map 
        for (auto iter = tc_server_->pending_blks.begin(); iter != tc_server_->pending_blks.end(); iter++)
        {
            bool is_found = false; 
            try {
                is_found = tc_server_->pending_blks.find(accessor, iter->first);

                if (is_found)
                {
                    std::shared_ptr<Block> blk = iter->second; 

                    msgpack::sbuffer b;
                    msgpack::pack(b, blk->header_); 
                    std::string blk_hdr_str = sbufferToString(b);

                    response->add_pb_hdrs(blk_hdr_str);
                }
            }
            catch (std::exception& e) {
                accessor.release(); 
                continue; 
            }
        }
        
        spdlog::debug("gRPC(PullPendingBlocks) ends"); 

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
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
    grpc::ServerUnaryReactor* GetBlocks(
        grpc::CallbackServerContext* context, 
        const GetBlocksRequest* request,
        GetBlocksResponse* response
    ) override
    {
        spdlog::debug("gRPC(GetBlocks) starts");

        response->set_status(0);

        auto req_blk_hdr = request->pb_hdrs(); 
        BlockCHM::accessor accessor;

        // unsafe interations on concurrent hash map 
        // but it is serial
        for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
        {
            // deserialize requested block headers 
            spdlog::trace("deserialize requested block headers");
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto blk_hdr = oh->as<BlockHeader>();

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
            spdlog::trace("serialize block"); 
            msgpack::sbuffer b;
            msgpack::pack(b, block); 
            std::string ser_blk = sbufferToString(b);

            // add serialized block to response
            spdlog::trace("add serialized block to response"); 
            response->add_pb(ser_blk); 
        }

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
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
    grpc::ServerUnaryReactor* VoteBlocks(
        grpc::CallbackServerContext* context, 
        const VoteBlocksRequest* request,
        VoteBlocksResponse* response
    ) override
    {
        spdlog::debug("gRPC(VoteBlocks) starts");

        response->set_status(0);

        auto client_id = request->id();
        auto voted_blocks = request->voted_blocks(); 
        spdlog::trace("{}:voted blocks count={}",
            client_id, 
            voted_blocks.size()
        ); 

        // unsafe interations on concurrent hash map 
        // but it is serial
        BlockCHM::accessor accessor;
        for (auto iter = voted_blocks.begin(); iter != voted_blocks.end(); iter++)
        {
            // deserialize request 
            spdlog::trace("{}:deserialize request", client_id);
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto block = oh->as<std::shared_ptr<Block>>();

            // get block vote from request 
            spdlog::trace("{}:get block vote from request", client_id); 
            auto vote = block->votes_.find(request->id()); 
            if (vote == block->votes_.end())
            {
                spdlog::error("{}:vote not found", client_id); 
                continue;
            }

            // find local block storage 
            spdlog::trace("{}:find local block storage", client_id); 
            bool block_is_found = tc_server_->pending_blks.find(accessor, block->header_.id_); 
            if (!block_is_found)
            {
                spdlog::error("{}:block not found", client_id); 
                continue;
            }

            // insert received vote 
            spdlog::trace("{}:insert received vote", client_id); 
            accessor->second->votes_.insert(
                std::make_pair(
                    request->id(), 
                    vote->second
                )
            );

            // if votes count enough
            spdlog::trace("{}:check if votes count enough", client_id);
            if (accessor->second->votes_.size() >= (*::conf_data)["client-count"])
            {
                // populate signature set 
                spdlog::trace("{}:populate signature set", client_id);
                BLSSigShareSet sig_share_set(
                    (*::conf_data)["client-count"],
                    (*::conf_data)["client-count"]
                ); 

                // unsafe interations on concurrent hash map 
                // but it is locked by pb_accessor
                for (
                    auto vote_iter = accessor->second->votes_.begin(); 
                    vote_iter != accessor->second->votes_.end(); 
                    vote_iter++
                ) {
                    spdlog::trace("{}:serialize signature share", client_id); 
                    auto vote = vote_iter->second; 
                    auto str = vote_iter->second->sig_share_->toString(); 

                    spdlog::trace("{}:add signature share", client_id);
                    sig_share_set.addSigShare(
                        vote_iter->second->sig_share_
                    ); 
                }

                spdlog::trace("{}:check if enough votes", client_id);
                if (sig_share_set.isEnough())
                {
                    // merge signature
                    spdlog::trace("{}:merge signature", client_id); 
                    std::shared_ptr<BLSSignature> tss_sig = sig_share_set.merge(); 
                    accessor->second->tss_sig_ = tss_sig;

                    // insert block to committed
                    spdlog::trace("{}:insert block to committed", client_id);
                    tc_server_->committed_blks.insert(
                        accessor, 
                        block->header_.id_
                    ); 

                    // remove block from pending 
                    spdlog::trace("{}:remove block from pending", client_id); 
                    tc_server_->pending_blks.erase(block->header_.id_); 
                }
                else 
                {
                    spdlog::error("not enough votes");
                }
            }

            accessor.release();
        }

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }

public: 
    std::shared_ptr<TcConsensusImpl> consensus_;
};