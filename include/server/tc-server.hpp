#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

namespace tomchain {

class TcConsensusImpl final : public TcConsensus::Service {
    /**
     * @brief Client registers when it connects to server. 
     * 
     * @param context RPC context. 
     * @param request RPC request. 
     * @param response RPC response. 
     * @return grpc::Status RPC status. 
     */
    grpc::Status Register(
        grpc::ServerContext* context, 
        const RegisterRequest* request,
        RegisterResponse* response
    ) override
    {
        response->set_id(1);
        response->set_status(0); 
        return grpc::Status::OK; 
    }
};

class TcServer {

public: 
    /**
     * @brief Start the server. 
     * 
     */
    void start(); 

private: 
    std::unique_ptr<grpc::Server> service_; 

};

}