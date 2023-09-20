#include <condition_variable>
#include <mutex>
#include <memory>
#include <fstream>

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>
#include "msgpack_adapter.hpp"

std::shared_ptr<nlohmann::json> conf_data;

#include "client/tc-client.hpp"

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

        bool heartbeat_flag = false;
        t.setInterval(
            [&]() {
                if (heartbeat_flag == true) { return; }
                heartbeat_flag = true; 
                this->Heartbeat(); 
                heartbeat_flag = false; 
            },
            (*::conf_data)["heartbeat-interval"]
        );

        // bool chain_flag = false;
        // t.setInterval(
        //     [&]() {
        //         if (chain_flag == true) { return; }
        //         chain_flag = true; 
        //         this->PullPendingBlocks(); 
        //         this->GetBlocks(); 
        //         if (this->pending_blks.size() > 0) 
        //         { 
        //             this->VoteBlocks(); 
        //         } 
        //         chain_flag = false; 
        //     },
        //     (*::conf_data)["pull-pb-interval"]
        // );

        bool pull_flag = false; 
        t.setInterval(
            [&]() {
                if (pull_flag == true) { return; }
                pull_flag = true; 
                this->PullPendingBlocks(); 
                pull_flag = false; 
            },
            (*::conf_data)["pull-pb-interval"]
        );

        // std::thread pull_thread(
        //     [&]() {
        //         while (true)
        //         {
        //             this->PullPendingBlocks(); 
        //             this->GetBlocks(); 
        //         }
        //     }
        // ); 
        // pull_thread.detach(); 

        bool get_flag = false; 
        t.setInterval(
            [&]() {
                if (get_flag == true) { return; }
                get_flag = true; 
                this->GetBlocks(); 
                get_flag = false; 
            },
            (*::conf_data)["pull-pb-interval"]
        );

        // std::thread get_thread(
        //     [&]() {
        //         while (true)
        //         {
        //             this->GetBlocks(); 
        //         }
        //     }
        // ); 
        // get_thread.detach(); 

        bool vote_flag = false; 
        t.setInterval(
            [&]() {
                if (vote_flag == true) { return; }
                vote_flag = true; 
                if (!this->pending_blks.empty()) 
                { 
                    this->VoteBlocks(); 
                } 
                vote_flag = false; 
            },
            (*::conf_data)["pull-pb-interval"]
        );

        // std::thread vote_thread(
        //     [&]() {
        //         while (true)
        //         {
        //             this->VoteBlocks(); 
        //         }

        //     }
        // ); 
        // vote_thread.detach(); 

        while (true)
        {
            sleep(INT_MAX);
        }
    }

}

int main(const int argc, const char *argv[])
{
    spdlog::info("TomChain client starts. ");
    // spdlog::flush_every(std::chrono::seconds(3));

    // set CLI argument parser
    spdlog::trace("Parsing CLI arguments: argc={}", argc);
    for (int i = 0; i < argc; i++)
    {
        spdlog::trace("argv[{}]={}", i, argv[i]);
    }
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
    spdlog::info("Parsing JSON configuration file. ");
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

    spdlog::flush_on(spdlog::level::from_str(
            (*::conf_data)["log-level"])); 
    // set log level
    spdlog::set_level(
        spdlog::level::from_str(
            (*::conf_data)["log-level"]));

    // start profiler
    spdlog::info("Starting profiler");
    if ((*::conf_data)["profiler-enable"])
    {
        EASY_PROFILER_ENABLE; 
        Timer t;
        t.setTimeout(
            [&]() {
                std::string filename = 
                    std::string{"profile-client-"} + 
                    std::to_string((*::conf_data)["client-id"].template get<uint64_t>()) + 
                    std::string{".prof"}; 
                profiler::dumpBlocksToFile(filename.c_str());
            },
            20000
        );
    }
    if ((*::conf_data)["profiler-listen"])
    {
        profiler::startListen();
    }

    // start client
    uint32_t client_index = (*::conf_data)["client-id"].template get<uint32_t>() - 1;
    uint32_t server_select = (client_index % (*::conf_data)["grpc-server-count"].template get<uint32_t>()) + 1; 
    uint32_t grpc_server_port = (*::conf_data)["grpc-server-port"].template get<uint32_t>() + server_select; 
    std::string server_addr = 
        (*::conf_data)["grpc-server-ip"].template get<std::string>() + 
        std::string{":"} + 
        std::to_string(grpc_server_port); 
    spdlog::info("Using gRPC server {}", server_addr); 
    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            // (*::conf_data)["grpc-server-addr"],
            server_addr, 
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