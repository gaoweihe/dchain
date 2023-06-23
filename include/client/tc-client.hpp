#include <memory>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

namespace tomchain {

class TcClient {

public: 
    TcClient(std::shared_ptr<grpc::Channel> channel)
        :stub_(TcConsensus::NewStub(channel)) {};

public: 
    /**
     * @brief Start the client. 
     * 
     */
    void start(); 

    /**
     * @brief Stop the client. 
     * 
     */
    void stop(); 

public:
    /**
     * @brief Client registers when it connects to server. 
     * 
     * @return RegisterResponse Register status replied by server. 
     */
    RegisterResponse Register(); 

private: 
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}