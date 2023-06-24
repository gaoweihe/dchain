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
     */
    void Register(); 

    void Heartbeat(); 

public: 
    std::shared_ptr<ecdsa::Key> skey;
    std::shared_ptr<ecdsa::PubKey> pkey;
    uint32_t client_id;

private: 
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}