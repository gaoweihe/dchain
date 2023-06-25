#include <thread>
#include <fstream>

#include "server/tc-server.hpp"

#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>

std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

void TcServer::start(const std::string addr)
{
    std::thread t([&] {
        TcConsensusImpl consensus_service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            addr, 
            grpc::InsecureServerCredentials()
        ); 
        builder.RegisterService(&consensus_service);

        service_ = std::unique_ptr<grpc::Server>(builder.BuildAndStart());
        service_->Wait(); 
    });
    t.detach();
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
    conf_data = std::make_shared<nlohmann::json>(nlohmann::json::parse(fs));

    tomchain::TcServer server; 
    server.start((*conf_data)["grpc-listen-addr"]); 

    while(true) { 
        sleep(2);
        spdlog::info("server watchdog");
    }
    return 0;
}
