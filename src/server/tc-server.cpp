#include <thread>

#include "server/tc-server.hpp"

#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"

namespace tomchain {

void TcServer::start()
{
    std::thread t([&] {
        TcConsensusImpl consensus_service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            "localhost:2510", 
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
    parser.parse_args(argc, argv); 

    tomchain::TcServer server; 
    server.start(); 

    while(true) { 
        sleep(2);
        spdlog::info("server watchdog");
    }
    return 0;
}
