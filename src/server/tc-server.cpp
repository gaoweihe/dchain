#include "server/tc-server.hpp"

#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"

namespace tomchain {

void TcServer::start()
{
    TcConsensusImpl consensus_service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(
        "localhost:2510", 
        grpc::InsecureServerCredentials()
    ); 
    builder.RegisterService(&consensus_service);

    service_ = std::unique_ptr<grpc::Server>(builder.BuildAndStart());
}

}

int main(const int argc, const char* argv[])
{
    spdlog::info("TomChain Server Starts. "); 

    argparse::ArgumentParser parser("tc-server");
    parser.parse_args(argc, argv); 

    tomchain::TcServer server; 
    server.start(); 

    return 0;
}
