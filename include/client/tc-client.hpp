#include <memory>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "key.h"

namespace tomchain {

class TcClient {

public: 
    /**
     * @brief Construct a new TomChain Client object
     * 
     * @param channel gRPC channel
     */
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

    /**
     * @brief Run timers and routines. Will not exit during the whole lifecycle. 
     * 
     */
    void schedule(); 

public:
    /**
     * @brief Client registers when it connects to server. 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status Register(); 

    /**
     * @brief Client heartbeats. 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status Heartbeat(); 

    /**
     * @brief Client pull pending blocks. 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status PullPendingBlocks(); 

    /**
     * @brief Get the Blocks object
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status GetBlocks(); 

public: 
    std::shared_ptr<ecdsa::Key> skey;
    std::shared_ptr<ecdsa::PubKey> pkey;
    uint32_t client_id;

private: 
    /**
     * @brief gRPC service stub. 
     * 
     */
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}