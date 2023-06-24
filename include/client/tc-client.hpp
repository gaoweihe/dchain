#include <memory>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "key.h"

namespace tomchain {

class TcClient {

public: 
    TcClient(std::shared_ptr<grpc::Channel> channel);

public: 
    /**
     * @brief Initialize local variables. 
     * 
     */
    void init();

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

    void schedule(); 

public:
    /**
     * @brief Client registers when it connects to server. 
     * 
     * @return RegisterResponse Register status replied by server. 
     */
    RegisterResponse Register(); 

public: 
    std::shared_ptr<ecdsa::Key> skey;
    std::shared_ptr<ecdsa::PubKey> pkey;

private: 
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}