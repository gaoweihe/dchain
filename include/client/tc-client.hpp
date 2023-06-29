#ifndef TC_CLIENT_HDR
#define TC_CLIENT_HDR

#include <memory>

#include "entity/block.hpp" 
#include "entity/transaction.hpp" 

#include "HashMap.h"
#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"
#include "key.h"
#include "oneapi/tbb/concurrent_hash_map.h"

namespace tomchain {

typedef oneapi::tbb::concurrent_hash_map<
    uint32_t, uint64_t
> AccountCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Block>
> BlockCHM; 

class TcClient {

public: 
    /**
     * @brief Construct a new TomChain Client object
     * 
     * @param channel gRPC channel
     */
    TcClient(std::shared_ptr<grpc::Channel> channel);
    virtual ~TcClient(); 

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
    AccountCHM accounts;

private: 
    /**
     * @brief gRPC service stub. 
     * 
     */
    std::unique_ptr<TcConsensus::Stub> stub_;
};

}

#endif /* TC_CLIENT_HDR */