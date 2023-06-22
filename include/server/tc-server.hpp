#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

namespace tomchain {

class TcConsensusImpl final : public TcConsensus::Service {
    grpc::Status Register(
        grpc::ServerContext* context, 
        const RegisterRequest* request,
        RegisterResponse* reply
    ) override
    {
        reply->set_id(1);
        reply->set_status(0); 
        return grpc::Status::OK; 
    }
};

class TcServer {

public: 
    void start(); 

private: 
    std::unique_ptr<grpc::Server> service_; 

};

}