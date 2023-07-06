#ifndef TC_SERVER_HDR
#define TC_SERVER_HDR

#include <map>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "HashMap.h"
#include "key.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include <alpaca/alpaca.h>
#include "libBLS/libBLS.h"
#include <nlohmann/json.hpp>

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
    void start(const std::string addr); 

    void init_server(); 
    void init_client_profile(); 

    /**
     * @brief Server scheduler. 
     * 
     */
    void schedule(); 

    void generate_tx(uint64_t num_tx); 

    void pack_block(uint64_t num_tx, uint64_t num_block);

public: 
    uint64_t server_id;
    ClientCHM clients;
    BlockCHM pending_blks; 
    BlockCHM committed_blks; 
    TransactionCHM pending_txs;

private: 
    std::unique_ptr<grpc::Server> grpc_server_; 

};

}

#endif /* TC_SERVER_HDR */