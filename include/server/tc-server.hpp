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
    ClientCHM clients;
    BlockCHM pending_blks; 
    BlockCHM committed_blks; 
    TransactionCHM pending_txs;

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
    std::weak_ptr<TcServer> tc_server_; 

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
        std::shared_ptr<TcServer> shared_tc_server = tc_server_.lock();

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
        response->set_status(0);  
        auto pb = tc_server_.lock()->pending_blks; 
        for (auto iter = pb.begin(); iter != pb.end(); iter++)
        {
            std::shared_ptr<Block> blk = iter->second; 

            msgpack::sbuffer b;
            msgpack::pack(b, blk->header_); 
            std::string blk_hdr_str = sbufferToString(b);

            response->add_pb_hdrs(blk_hdr_str);
        }
        
        spdlog::info("pull pending blocks"); 

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

        auto pb = tc_server_.lock()->pending_blks; 

        auto req_blk_hdr = request->pb_hdrs(); 
        spdlog::info("req blk hdr size: {}", req_blk_hdr.size()); 
        for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
        {
            // deserialize requested block headers 
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto blk_hdr = oh->as<BlockHeader>();

            // find local blocks 
            BlockCHM::accessor accessor;
            bool is_found = pb.find(accessor, blk_hdr.id_); 
            spdlog::info("is found: {}", is_found); 
            std::shared_ptr<Block> block = accessor->second; 

            // serialize block 
            msgpack::sbuffer b;
            msgpack::pack(b, block); 
            std::string ser_blk = sbufferToString(b);

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
        spdlog::info("vote blocks"); 
        response->set_status(0);

        auto pb = tc_server_.lock()->pending_blks; 
        auto cb = tc_server_.lock()->committed_blks;

        auto voted_blocks = request->voted_blocks(); 
        spdlog::info("vb count: {}", voted_blocks.size()); 
        for (auto iter = voted_blocks.begin(); iter != voted_blocks.end(); iter++)
        {
            // deserialize request 
            msgpack::sbuffer des_b = stringToSbuffer(*iter);
            auto oh = msgpack::unpack(des_b.data(), des_b.size());
            auto block = oh->as<std::shared_ptr<Block>>();

            // get block vote from request 
            auto vote = block->votes_.find(request->id()); 

            // find local block storage 
            BlockCHM::accessor blk_accessor;
            bool is_found = pb.find(blk_accessor, block->header_.id_); 
            assert(is_found); 

            // insert received vote 
            blk_accessor->second->votes_.insert(
                std::make_pair(
                    request->id(), 
                    vote->second
                )
            );

            // if votes count enough
            if (blk_accessor->second->votes_.size() >= (*::conf_data)["client-count"])
            {
                // populate signature set 
                BLSSigShareSet sig_share_set(
                    (*::conf_data)["client-count"],
                    (*::conf_data)["client-count"]
                ); 

                for (auto vote_iter = blk_accessor->second->votes_.begin(); vote_iter != blk_accessor->second->votes_.end(); vote_iter++)
                {
                    auto vote = vote_iter->second; 
                    auto str = vote_iter->second->sig_share_->toString(); 

                    sig_share_set.addSigShare(
                        vote_iter->second->sig_share_
                    ); 
                }
                
                // assert that signature should be enough 
                assert(sig_share_set.isEnough()); 

                if (sig_share_set.isEnough())
                {
                    // merge signature
                    std::shared_ptr<BLSSignature> tss_sig = sig_share_set.merge(); 
                    blk_accessor->second->tss_sig_ = tss_sig;

                    // move block from pending to committed
                    cb.insert(
                        blk_accessor, 
                        block->header_.id_
                    ); 

                    blk_accessor.release(); 
                    pb.erase(block->header_.id_); 

                    spdlog::info("committed block: {}", block->header_.id_); 
                    spdlog::info("pb: {}, cb: {}", pb.size(), cb.size()); 
                }
            }
        }

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }

public: 
    std::shared_ptr<TcConsensusImpl> consensus_;
};

}

#endif /* TC_SERVER_HDR */