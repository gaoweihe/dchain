#include <memory>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

namespace tomchain {

class TcClient {

public: 
    TcClient(std::shared_ptr<grpc::Channel> channel)
        :stub_(TcConsensus::NewStub(channel)) {};

private: 
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}