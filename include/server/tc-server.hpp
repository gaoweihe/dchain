#include <map>

#include "spdlog/spdlog.h"
#include "HashMap.h"
#include "key.h"

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "block.hpp" 
#include "transaction.hpp" 

namespace tomchain {

class TcConsensusImpl final : public TcConsensus::CallbackService {

public:
    // TcConsensusImpl::TcConsensusImpl() 
    // {

    // }

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
        std::string pkey_str = request->pkey();
        std::vector<uint8_t> pkey_data_vec(pkey_str.begin(), pkey_str.end());
        ecdsa::PubKey pkey(pkey_data_vec);  
        // tc_server->clients.insert(pkey, 1); 

        response->set_id(1);
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

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }
};

class TcServer {

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

    /**
     * @brief Server scheduler. 
     * 
     */
    void schedule(); 

public: 
    CTSL::HashMap<uint32_t, ecdsa::PubKey> clients;
    CTSL::HashMap<uint32_t, Block> pedning_blks; 
    CTSL::HashMap<uint32_t, Transaction> pedning_txs; 

private: 
    std::unique_ptr<grpc::Server> service_; 

};

}