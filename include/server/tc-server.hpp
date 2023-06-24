#include <map>

#include "spdlog/spdlog.h"
#include "HashMap.h"

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

namespace tomchain {

class TcConsensusImpl final : public TcConsensus::CallbackService {
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
        response->set_id(1);
        response->set_status(0); 
        spdlog::info("register"); 

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }
};

class TcServer {

public: 
    /**
     * @brief Start the server. 
     * 
     */
    void start(); 

public: 
    CTSL::HashMap<uint32_t, uint32_t> clients;

private: 
    std::unique_ptr<grpc::Server> service_; 

};

}