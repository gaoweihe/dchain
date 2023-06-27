#include <thread>
#include <fstream>
#include <random>

#include "server/tc-server.hpp"

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>

static std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

TcServer::TcServer() 
{

}

TcServer::~TcServer() 
{
    
}

void TcServer::start(const std::string addr)
{
    auto self(shared_from_this()); 
    std::thread grpc_thread([&]() {
        TcConsensusImpl consensus_service; 
        consensus_service.tc_server_ = self;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            addr, 
            grpc::InsecureServerCredentials()
        ); 
        builder.RegisterService(&consensus_service);

        grpc_server_ = builder.BuildAndStart();
        grpc_server_->Wait(); 
    });
    grpc_thread.detach();

    std::thread schedule_thread([&] {
        this->schedule(); 
    }); 
    schedule_thread.detach();
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
            0, 
            0, 
            0, 
            0
        ); 
        pending_txs.insert(tx_id, tx);
    }
}

void TcServer::pack_block(uint64_t num_tx, uint64_t num_block)
{
    
}

}

int main(const int argc, const char* argv[])
{
    spdlog::info("TomChain server starts. "); 

    argparse::ArgumentParser parser("tc-server");
    parser.add_argument("--cf")
        .help("configuration file")
        .required()
        .default_value(std::string{""}); 
    parser.parse_args(argc, argv); 
    
    std::string conf_file_path = parser.get<std::string>("--cf");
    std::ifstream fs(conf_file_path);
    ::conf_data = std::make_shared<nlohmann::json>(nlohmann::json::parse(fs));

    tomchain::TcServer server; 
    // tc_server = std::make_shared<tomchain::TcServer>(server); 
    server.start((*::conf_data)["grpc-listen-addr"]); 

    while(true) { 
        sleep(2);
        spdlog::info("server watchdog");
    }
    return 0;
}
