#ifndef TC_SERVER_HDR
#define TC_SERVER_HDR

#include <map>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "HashMap.h"
#include "key.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/concurrent_queue.h"
#include <alpaca/alpaca.h>
#include "libBLS/libBLS.h"
#include <nlohmann/json.hpp>
#include <easy/profiler.h>

#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"
#include "tc-server-peer.grpc.pb.h"

#include "block.hpp" 
#include "transaction.hpp" 
#include "msgpack_adapter.hpp"

extern std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

class ClientProfile {
public: 
    uint64_t id;
    std::shared_ptr<ecdsa::PubKey> ecc_pkey;
    std::shared_ptr<std::pair<
        std::shared_ptr<BLSPrivateKeyShare>, 
        std::shared_ptr<BLSPublicKeyShare>
    >> tss_key;
};

typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<ClientProfile>
> ClientCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Transaction>
> TransactionCHM; 
typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<Block>
> BlockCHM; 

class TcServer : 
    virtual public std::enable_shared_from_this<TcServer> {

public: 
    TcServer(); 
    virtual ~TcServer(); 

public: 
    /**
     * @brief Starts the server. 
     * 
     * @param addr Listen address. 
     */
    void start(); 

    void init_server(); 
    void init_client_profile(); 
    void init_peer_stubs(); 

    /**
     * @brief Server scheduler. 
     * 
     */
    void schedule(); 

    void generate_tx(uint64_t num_tx); 

    void pack_block(uint64_t num_tx, uint64_t num_block);

public: 
    void send_relay_votes(); 
    void send_relay_blocks(); 
    grpc::Status RelayVote(uint64_t target_server_id); 
    grpc::Status RelayBlock(uint64_t target_server_id); 
    void send_heartbeats(); 
    grpc::Status SPHeartbeat(uint64_t target_server_id); 

public: 
    uint64_t server_id;
    ClientCHM clients;
    BlockCHM pending_blks; 
    BlockCHM committed_blks; 
    TransactionCHM pending_txs;
    std::map<
        uint64_t, 
        std::shared_ptr<
            oneapi::tbb::concurrent_queue<
                std::shared_ptr<BlockVote>
            >
        >
    > relay_votes; 
    std::map<
        uint64_t, 
        std::shared_ptr<
            oneapi::tbb::concurrent_queue<
                std::shared_ptr<Block>
            >
        >
    > relay_blocks; 


private: 
    std::unique_ptr<grpc::Server> grpc_server_; 
    std::unique_ptr<grpc::Server> grpc_peer_server_; 
    std::map<
        uint64_t, 
        std::unique_ptr<TcPeerConsensus::Stub>
    > grpc_peer_client_stub_;

};

}

#endif /* TC_SERVER_HDR */