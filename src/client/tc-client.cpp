#include <condition_variable>
#include <mutex>
#include <memory>
#include <fstream>

#include "client/tc-client.hpp"

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>
#include "msgpack_adapter.hpp"

std::shared_ptr<nlohmann::json> conf_data;

// gRPC implementations
#include "client/tc-client-grpc.hpp"

namespace tomchain
{

    TcClient::TcClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(TcConsensus::NewStub(channel))
    {
        this->init();
    }

    TcClient::~TcClient()
    {
    }

    void TcClient::init()
    {
        this->client_id = (*::conf_data)["client-id"];
        this->ecc_skey = std::make_shared<ecdsa::Key>(ecdsa::Key());
        this->ecc_pkey = std::make_shared<ecdsa::PubKey>(
            this->ecc_skey->CreatePubKey());
    }

    void TcClient::start()
    {
        this->Register();
        this->schedule();
    }

    void TcClient::schedule()
    {
        Timer t;

        t.setInterval([&]()
                      { this->Heartbeat(); },
                      (*::conf_data)["heartbeat-interval"]);

        t.setInterval([&]()
                      {
        this->PullPendingBlocks(); 
        this->GetBlocks(); 
        this->VoteBlocks(); },
                      (*::conf_data)["pull-pb-interval"]);

        while (true)
        {
            sleep(INT_MAX);
        }
    }

}

int main(const int argc, const char *argv[])
{
    spdlog::info("TomChain client starts. ");

    // set CLI argument parser
    argparse::ArgumentParser parser("tc-client");
    parser.add_argument("--cf")
        .help("configuration file")
        .required()
        .default_value(std::string{""});
    parser.add_argument("--id")
        .help("client id")
        .scan<'u', uint32_t>()
        .default_value(1);
    parser.parse_args(argc, argv);

    // parse json configuration
    std::string conf_file_path = parser.get<std::string>(
        "--cf");
    std::ifstream fs(conf_file_path);
    ::conf_data = std::make_shared<nlohmann::json>(
        nlohmann::json::parse(fs));

    uint64_t conf_client_id = parser.get<uint32_t>("--id");
    if (conf_client_id != 0)
    {
        (*::conf_data)["client-id"] = conf_client_id;
    }

    // set log level
    spdlog::set_level(
        spdlog::level::from_str(
            (*::conf_data)["log-level"]));

    // start profiler
    spdlog::info("Starting profiler");
    if ((*::conf_data)["profiler-enable"])
    {
        EASY_PROFILER_ENABLE; 
    }
    if ((*::conf_data)["profiler-listen"])
    {
        profiler::startListen();
    }

    

    // start client
    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            (*::conf_data)["grpc-server-addr"],
            grpc::InsecureChannelCredentials()));
    tcClient.start();

    // watch dog
    while (true)
    {
        sleep(2);
        spdlog::info("client watchdog");
    }
    return 0;
}