#ifndef TC_SERVER_HDR
#define TC_SERVER_HDR

#include <map>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "HashMap.h"
#include "key.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include <alpaca/alpaca.h>
#include "libBLS/libBLS.h"
#include <nlohmann/json.hpp>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "block.hpp" 
#include "transaction.hpp" 
#include "msgpack_packer.hpp"

extern std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

class ClientProfile {
public: 
    uint64_t id;
    std::shared_ptr<ecdsa::PubKey> ecc_pkey;
    std::shared_ptr<std::pair<
        std::shared_ptr<BLSPrivateKeyShare>, 
        std::shared_ptr<BLSPublicKeyShare>
    >> tss_key;
};

typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<ClientProfile>
> ClientCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Transaction>
> TransactionCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Block>
> BlockCHM; 

class TcServer : 
    virtual public std::enable_shared_from_this<TcServer> {

public: 
    TcServer(); 
    virtual ~TcServer(); 

public: 
    /**
     * @brief Starts the server. 
     * 
     * @param addr Listen address. 
     */
    void start(const std::string addr); 

    void init_client_profile(); 

    /**
     * @brief Server scheduler. 
     * 
     */
    void schedule(); 

    void generate_tx(uint64_t num_tx); 

    void pack_block(uint64_t num_tx, uint64_t num_block);

public: 
    uint64_t uuid; 
    ClientCHM clients;
    BlockCHM pending_blks; 
    BlockCHM committed_blks; 
    TransactionCHM pending_txs;
    BlockCHM::accessor pb_accessor; 
    uint64_t block_id_count = 0; 

private: 
    std::unique_ptr<grpc::Server> grpc_server_; 

};

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
        spdlog::info("register"); 

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
        response->set_status(0); 
        spdlog::info("heartbeat"); 

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
        uint64_t tmp_id = 0;

        response->set_status(0);  
        // auto pb = tc_server_->pending_blks; 
        for (auto iter = tc_server_->pending_blks.begin(); iter != tc_server_->pending_blks.end(); iter++)
        {
            std::shared_ptr<Block> blk = iter->second; 

            tmp_id = blk->header_.id_;

            msgpack::sbuffer b;
            msgpack::pack(b, blk->header_); 
            std::string blk_hdr_str = sbufferToString(b);

            response->add_pb_hdrs(blk_hdr_str);
        }
        
        spdlog::info("pull pending blocks"); 

        // tc_server_->pending_blks.erase(tmp_id); 

        spdlog::info("pb count: {}@{}@{}", tc_server_->pending_blks.size(), tc_server_->uuid, (uint64_t)(tc_server_.get()));

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
        spdlog::info("get blocks"); 

        response->set_status(0);

        // auto pb = tc_server_->pending_blks; 

        auto req_blk_hdr = request->pb_hdrs(); 
        for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
        {
            // deserialize requested block headers 
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto blk_hdr = oh->as<BlockHeader>();

            // find local blocks 
            // BlockCHM::accessor accessor;
            tc_server_->pending_blks.find(tc_server_->pb_accessor, blk_hdr.id_); 
            std::shared_ptr<Block> block = tc_server_->pb_accessor->second; 

            // serialize block 
            msgpack::sbuffer b;
            msgpack::pack(b, block); 
            std::string ser_blk = sbufferToString(b);

            response->add_pb(ser_blk); 

            tc_server_->pb_accessor.release(); 
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
        spdlog::info("vote blocks"); 
        response->set_status(0);

        // auto pb = tc_server_->pending_blks; 
        auto cb = tc_server_->committed_blks;

        auto voted_blocks = request->voted_blocks(); 
        spdlog::info("vb count: {}", voted_blocks.size()); 
        for (auto iter = voted_blocks.begin(); iter != voted_blocks.end(); iter++)
        {
            // deserialize request 
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto block = oh->as<std::shared_ptr<Block>>();

            /* Additional testing code */
            auto block_id = block->header_.id_;
            tc_server_->pending_blks.find(tc_server_->pb_accessor, block_id);
            tc_server_->pending_blks.erase(tc_server_->pb_accessor);
            spdlog::info("erase: {}", block_id); 

            // // get block vote from request 
            // auto vote = block->votes_.find(request->id()); 

            // // find local block storage 
            // // BlockCHM::accessor blk_accessor;
            // pb.find(tc_server_->pb_accessor, block->header_.id_); 

            // // insert received vote 
            // tc_server_->pb_accessor->second->votes_.insert(
            //     std::make_pair(
            //         request->id(), 
            //         vote->second
            //     )
            // );

            // // if votes count enough
            // if (tc_server_->pb_accessor->second->votes_.size() >= (*::conf_data)["client-count"])
            // {
            //     // populate signature set 
            //     BLSSigShareSet sig_share_set(
            //         (*::conf_data)["client-count"],
            //         (*::conf_data)["client-count"]
            //     ); 

            //     for (auto vote_iter = tc_server_->pb_accessor->second->votes_.begin(); vote_iter != tc_server_->pb_accessor->second->votes_.end(); vote_iter++)
            //     {
            //         auto vote = vote_iter->second; 
            //         auto str = vote_iter->second->sig_share_->toString(); 

            //         sig_share_set.addSigShare(
            //             vote_iter->second->sig_share_
            //         ); 
            //     }
                
            //     // assert that signature should be enough 
            //     assert(sig_share_set.isEnough()); 

            //     if (sig_share_set.isEnough())
            //     {
            //         // merge signature
            //         std::shared_ptr<BLSSignature> tss_sig = sig_share_set.merge(); 
            //         tc_server_->pb_accessor->second->tss_sig_ = tss_sig;

            //         // move block from pending to committed
            //         // cb.insert(
            //         //     blk_accessor, 
            //         //     block->header_.id_
            //         // ); 

            //         // pb.erase(block->header_.id_); 
            //         pb.erase(tc_server_->pb_accessor); 
            //     }
            // }

            // tc_server_->pb_accessor.release();             
        }

        spdlog::info("pb count: {}@{}@{}", tc_server_->pending_blks.size(), tc_server_->uuid, (uint64_t)(tc_server_.get()));

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }

public: 
    std::shared_ptr<TcConsensusImpl> consensus_;
};

}

#endif /* TC_SERVER_HDR */