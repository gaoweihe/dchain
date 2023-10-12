#include <condition_variable>
#include <mutex>
#include <memory>
#include <fstream>
#include <thread>
#include <pthread.h>

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

    TcClient::TcClient(std::shared_ptr<grpc::Channel> channel, std::shared_ptr<grpc::Channel> shadow_channel)
    {
        stubs.push_back(TcConsensus::NewStub(channel));
        stubs.push_back(TcConsensus::NewStub(shadow_channel));

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

        rocksdb::Options options;
        options.create_if_missing = true;
        std::string rocksdb_filename = std::string{"/tmp/tomchain/tc-client"} + "-" + std::to_string((*::conf_data)["client-id"].template get<uint64_t>());
        rocksdb::Status status =
            rocksdb::DB::Open(options, rocksdb_filename.c_str(), &db);
        assert(status.ok());
    }

    void TcClient::start()
    {
        this->Register(0);
        this->schedule();
    }

    void TcClient::schedule()
    {
        Timer t;

        bool heartbeat_flag = false;
        // t.setInterval(
        //     [&]() {
        //         if (heartbeat_flag == true) { return; }
        //         heartbeat_flag = true;
        //         this->Heartbeat();
        //         heartbeat_flag = false;
        //     },
        //     (*::conf_data)["heartbeat-interval"]
        // );

        std::thread heartbeat_thread_handle = std::thread(
            [&]()
            {
                const auto processor_count = std::thread::hardware_concurrency();
                const auto bind_core = (*::conf_data)["client-id"].template get<uint64_t>() % processor_count;
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(bind_core, &cpuset);
                int rc = pthread_setaffinity_np(
                    heartbeat_thread_handle.native_handle(),
                    sizeof(cpu_set_t),
                    &cpuset);
                if (rc != 0)
                {
                    spdlog::error("Error calling pthread_setaffinity_np: {}", rc);
                    exit(-1);
                }

                // sleep ms
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                while (true)
                {
                    if (heartbeat_flag == true)
                    {
                        return;
                    }
                    heartbeat_flag = true;
                    this->Heartbeat(0);
                    heartbeat_flag = false;

                    // sleep ms
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(
                            (*::conf_data)["heartbeat-interval"].template get<uint64_t>()));
                }
            });

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
        // t.setInterval(
        //     [&]()
        //     {
        //         if (pull_flag == true)
        //         {
        //             return;
        //         }
        //         pull_flag = true;
        //         this->PullPendingBlocks();
        //         this->GetBlocks();
        //         pull_flag = false;
        //     },
        //     (*::conf_data)["pull-pb-interval"]);

        std::thread pull_thread_handle = std::thread(
            [&]()
            {
                const auto processor_count = std::thread::hardware_concurrency();
                const auto bind_core = (*::conf_data)["client-id"].template get<uint64_t>() % processor_count;
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(bind_core, &cpuset);
                int rc = pthread_setaffinity_np(
                    heartbeat_thread_handle.native_handle(),
                    sizeof(cpu_set_t),
                    &cpuset);
                if (rc != 0)
                {
                    spdlog::error("Error calling pthread_setaffinity_np: {}", rc);
                    exit(-1);
                }

                // sleep ms
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                while (true)
                {
                    if (pull_flag == true)
                    {
                        return;
                    }
                    pull_flag = true;
                    this->PullPendingBlocks(0);
                    this->PullPendingBlocks(1);
                    this->GetBlocks(0);
                    this->GetBlocks(1);
                    pull_flag = false;

                    // sleep ms
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(
                            (*::conf_data)["pull-pb-interval"].template get<uint64_t>()));
                }
            });

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

        // bool get_flag = false;
        // t.setInterval(
        //     [&]() {
        //         if (get_flag == true) { return; }
        //         get_flag = true;
        //         this->GetBlocks();
        //         get_flag = false;
        //     },
        //     (*::conf_data)["pull-pb-interval"]
        // );

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
        // t.setInterval(
        //     [&]()
        //     {
        //         if (vote_flag == true)
        //         {
        //             return;
        //         }
        //         vote_flag = true;
        //         if (!this->pending_blks.empty())
        //         {
        //             this->VoteBlocks();
        //         }
        //         vote_flag = false;
        //     },
        //     (*::conf_data)["pull-pb-interval"]);

        std::mutex vote_mutex;
        std::thread vote_thread_handle = std::thread(
            [&]()
            {
                const auto processor_count = std::thread::hardware_concurrency();
                const auto bind_core = (*::conf_data)["client-id"].template get<uint64_t>() % processor_count;
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(bind_core, &cpuset);
                int rc = pthread_setaffinity_np(
                    heartbeat_thread_handle.native_handle(),
                    sizeof(cpu_set_t),
                    &cpuset);
                if (rc != 0)
                {
                    spdlog::error("Error calling pthread_setaffinity_np: {}", rc);
                    exit(-1);
                }

                // sleep ms
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                while (true)
                {
                    if (vote_flag == true)
                    {
                        return;
                    }
                    vote_flag = true;
                    this->VoteBlocks();
                    vote_flag = false;

                    // sleep ms
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(
                            (*::conf_data)["pull-pb-interval"].template get<uint64_t>()));
                }
            });

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
            [&]()
            {
                std::string filename =
                    std::string{"profile-client-"} +
                    std::to_string((*::conf_data)["client-id"].template get<uint64_t>()) +
                    std::string{".prof"};
                profiler::dumpBlocksToFile(filename.c_str());
            },
            20000);
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

    uint32_t shadow_server_select = (client_index % (*::conf_data)["grpc-server-count"].template get<uint32_t>()) + 2;
    // if last server, select first server as shadow
    if (shadow_server_select > (*::conf_data)["grpc-server-count"].template get<uint32_t>())
    {
        shadow_server_select = 1;
    }
    uint32_t grpc_shadow_server_port = (*::conf_data)["grpc-server-port"].template get<uint32_t>() + shadow_server_select;
    std::string shadow_server_addr =
        (*::conf_data)["grpc-server-ip"].template get<std::string>() +
        std::string{":"} +
        std::to_string(grpc_shadow_server_port);
    spdlog::info("Using gRPC shadow server {}", shadow_server_addr);

    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            // (*::conf_data)["grpc-server-addr"],
            server_addr,
            grpc::InsecureChannelCredentials()),
        grpc::CreateChannel(
            // (*::conf_data)["grpc-server-addr"],
            shadow_server_addr,
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