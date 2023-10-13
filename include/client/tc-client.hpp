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
#include "oneapi/tbb/concurrent_queue.h"
#include "libBLS/libBLS.h"
#include <easy/profiler.h>
#include "rocksdb/db.h"

namespace tomchain {

typedef oneapi::tbb::concurrent_hash_map<
    uint32_t, uint64_t
> AccountCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Block>
> BlockCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<BlockHeader>
> BlockHeaderCHM; 

class TcClient {

public: 
    /**
     * @brief Construct a new TomChain Client object
     * 
     * @param channel gRPC channel
     */
    TcClient(std::shared_ptr<grpc::Channel> channel, std::shared_ptr<grpc::Channel> shadow_channel);
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
    grpc::Status Register(uint64_t stub_id); 

    /**
     * @brief Client heartbeats. 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status Heartbeat(uint64_t stub_id); 

    /**
     * @brief Client pull pending blocks. 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status PullPendingBlocks(uint64_t stub_id); 

    /**
     * @brief Get the Blocks object
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status GetBlocks(uint64_t stub_id); 

    /**
     * @brief Vote for blocks by sending signature shares 
     * 
     * @return grpc::Status RPC status. 
     */
    grpc::Status VoteBlocks(); 

public: 
    std::shared_ptr<ecdsa::Key> ecc_skey;
    std::shared_ptr<ecdsa::PubKey> ecc_pkey;
    uint64_t client_id;
    AccountCHM accounts;
    oneapi::tbb::concurrent_queue<
        std::shared_ptr<Block>
    > pending_blks;
    BlockHeaderCHM pending_blkhdr;
    std::shared_ptr<std::pair<
        std::shared_ptr<BLSPrivateKeyShare>, 
        std::shared_ptr<BLSPublicKeyShare>
    >> tss_key;
    BlockCHM voted_blks; 
    BlockCHM committed_blks; 
    std::mutex db_mutex; 
    rocksdb::DB* db;

private: 
    /**
     * @brief gRPC service stub. 
     * 
     */
    std::vector<std::unique_ptr<TcConsensus::Stub>> stubs; 

};

}

#endif /* TC_CLIENT_HDR */