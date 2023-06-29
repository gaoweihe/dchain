#ifndef TC_SERVER_HDR
#define TC_SERVER_HDR

#include <map>
#include <memory>

#include "spdlog/spdlog.h"
#include "HashMap.h"
#include "key.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include <alpaca/alpaca.h>
#include "libBLS/libBLS.h"

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "block.hpp" 
#include "transaction.hpp" 

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
            auto blk = iter->second; 
            std::stringstream ss; 
            ss << bits(blk->header_);
            auto blk_hdr_str = ss.str(); 
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
        response->set_status(0); 
        spdlog::info("get blocks"); 

        auto pb = tc_server_.lock()->pending_blks; 

        response->set_status(0);
        
        auto req_blk_hdr = request->pb_hdrs(); 
        for (auto iter = req_blk_hdr.begin(); iter != req_blk_hdr.end(); iter++)
        {
            std::istringstream iss(*iter);
            BlockHeader blk_hdr;
            iss >> bits(blk_hdr); 
            BlockCHM::const_accessor accessor;
            pb.find(accessor, blk_hdr.id_); 
            const std::shared_ptr<tomchain::Block> block = accessor->second; 

            std::stringstream ss; 
            ss << block; 
            std::string ser_blk = ss.str(); 
            response->add_pb(ser_blk); 
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