#include <thread>
#include <fstream>
#include <random>

#include "server/tc-server.hpp"

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"

std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

TcServer::TcServer() 
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<
        std::mt19937::result_type
    > distribution(1, INT_MAX);
    uuid = distribution(rng);
}

TcServer::~TcServer() 
{
    
}

void TcServer::start(const std::string addr)
{
    this->init_client_profile(); 
    
    std::thread grpc_thread([&]() {
        std::shared_ptr<TcServer> shared_from_tc_server = shared_from_this(); 
        std::shared_ptr<TcConsensusImpl> consensus_service = 
            std::make_shared<TcConsensusImpl>(); 
        consensus_service->tc_server_ = shared_from_tc_server;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            addr, 
            grpc::InsecureServerCredentials()
        ); 
        builder.RegisterService(consensus_service.get());

        grpc_server_ = builder.BuildAndStart();
        grpc_server_->Wait(); 
    });
    grpc_thread.detach();

    std::thread schedule_thread([&] {
        this->schedule(); 
    }); 
    schedule_thread.detach();
}

void TcServer::init_client_profile()
{
    auto client_count = (*::conf_data)["client-count"]; 

    DKGBLSWrapper dkg_obj(client_count, client_count);
    std::shared_ptr<std::vector<libff::alt_bn128_Fr>> sshares = dkg_obj.createDKGSecretShares(); 

    // client number starts from one
    for (size_t i = 0; i < client_count; i++)
    {
        ClientProfile client_profile;
        client_profile.id = i + 1;

        // TSS keys 
        BLSPrivateKeyShare skey_share(sshares->at(i), client_count, client_count);
        BLSPublicKeyShare pkey_share(sshares->at(i), client_count, client_count); 
        client_profile.tss_key = std::make_shared<std::pair<
            std::shared_ptr<BLSPrivateKeyShare>, 
            std::shared_ptr<BLSPublicKeyShare>
        >>(
            std::make_pair<
                std::shared_ptr<BLSPrivateKeyShare>, 
                std::shared_ptr<BLSPublicKeyShare>
            >(
                std::make_shared<BLSPrivateKeyShare>(skey_share), 
                std::make_shared<BLSPublicKeyShare>(pkey_share)
            )
        );

        clients.insert(
            std::make_pair(
                client_profile.id, 
                std::make_shared<ClientProfile>(client_profile)
            )
        );
    }
}

void TcServer::schedule()
{
    Timer t;

    // generate transactions
    t.setInterval([&]() {
        // number of generated transactions per second 
        const uint64_t gen_tx_rate = (*::conf_data)["generate-tx-rate"]; 
        this->generate_tx(gen_tx_rate);
    }, (*::conf_data)["scheduler_freq"]); 

    // pack blocks 
    t.setInterval([&]() {
        // number of generated transactions per second 
        const uint64_t tx_per_block = (*::conf_data)["tx-per-block"]; 
        this->pack_block(tx_per_block, INT_MAX); 
    }, (*::conf_data)["scheduler_freq"]); 

    // count blocks
    t.setInterval([&]() {
        spdlog::info("tx:{} | pb:{} | cb:{}", 
            pending_txs.size(), 
            pending_blks.size(), 
            committed_blks.size()
        );
    }, (*::conf_data)["scheduler_freq"]); 

    // TODO: change to shutdown conditional variable 
    while(true) { sleep(INT_MAX); }
}

void TcServer::generate_tx(uint64_t num_tx)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<
        std::mt19937::result_type
    > distribution(1, INT_MAX);

    for (size_t i = 0; i < num_tx; i++)
    {
        uint64_t tx_id = distribution(rng);
        Transaction tx(
            tx_id, 
            0xDEADBEEF, 
            0xDEADBEEF, 
            0xDEADBEEF, 
            0xDEADBEEF
        ); 
        pending_txs.insert(
            std::make_pair(tx_id, std::make_shared<Transaction>(tx))
        ); 
    }
}

void TcServer::pack_block(uint64_t num_tx, uint64_t num_block)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<
        std::mt19937::result_type
    > distribution(1, INT_MAX);

    for (size_t i = 0; i < num_block; i++)
    {
        std::size_t pending_txs_size = pending_txs.size(); 
        if (pending_txs_size >= num_tx) 
        {
            std::vector<uint64_t> extracted_tx; 
            TransactionCHM::iterator it; 

            // Construct new block 
            uint64_t block_id = distribution(rng);
            block_id = block_id_count++;
            // TODO: base id
            Block new_block(block_id, 0xDEADBEEF);
            for(it = pending_txs.begin(); it != pending_txs.end(); ++it)
            {
                extracted_tx.push_back(it->first); 
                new_block.tx_vec_.push_back(it->second); 
            }

            pending_blks.insert(
                pb_accessor, 
                block_id
            ); 
            pb_accessor->second = std::make_shared<Block>(new_block); 
            spdlog::info("gen block: {}", block_id); 

            // remove extracted pending transactions 
            for (auto iter = extracted_tx.begin(); iter < extracted_tx.end(); iter++)
            {
                pending_txs.erase(*iter); 
            }
        }
        else {
            break; 
        }
    }
}

}

int main(const int argc, const char* argv[])
{
    spdlog::info("TomChain server starts. "); 

    // set CLI argument parser
    argparse::ArgumentParser parser("tc-server");
    parser.add_argument("--cf")
        .help("configuration file")
        .required()
        .default_value(std::string{""}); 
    parser.parse_args(argc, argv); 
    
    // parse json configuration
    std::string conf_file_path = parser.get<std::string>("--cf");
    std::ifstream fs(conf_file_path);
    ::conf_data = std::make_shared<nlohmann::json>(nlohmann::json::parse(fs));

    // set log level
    spdlog::set_level(
        spdlog::level::from_str(
            (*::conf_data)["log-level"]
        )
    );

    // start server
    std::shared_ptr<tomchain::TcServer> server = 
        std::make_shared<tomchain::TcServer>(); 
    server->start((*::conf_data)["grpc-listen-addr"]); 

    // watch dog
    while(true) { 
        sleep(2);
        spdlog::info("server watchdog");
    }
    return 0;
}
