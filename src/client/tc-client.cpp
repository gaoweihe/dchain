#include <condition_variable>
#include <mutex>
#include <memory>
#include <fstream>

#include "client/tc-client.hpp"

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>

std::shared_ptr<nlohmann::json> conf_data; 

namespace tomchain {

TcClient::TcClient(std::shared_ptr<grpc::Channel> channel)
    :stub_(TcConsensus::NewStub(channel)) 
{
    this->init(); 
}

TcClient::~TcClient() 
{

}

grpc::Status TcClient::Register()
{
    RegisterRequest request; 
    const std::vector<uint8_t>& ser_pkey = this->pkey->get_pub_key_data();
    request.set_pkey(ser_pkey.data(), ser_pkey.size());
    RegisterResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->Register(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    spdlog::info("register: {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::Heartbeat()
{
    HeartbeatRequest request; 
    request.set_id(this->client_id); 
    HeartbeatResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->Heartbeat(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    spdlog::info("heartbeat: {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::PullPendingBlocks()
{
    PullPendingBlocksRequest request; 
    request.set_id(this->client_id); 
    PullPendingBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->PullPendingBlocks(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    // unpack response 
    for (int i = 0; i < response.pb_hdrs_size(); i++) {
        std::istringstream block_hdr_ss(
            response.pb_hdrs(i)
        ); 

        BlockHeader block_hdr;
        block_hdr_ss >> bits(block_hdr);
        spdlog::info("block id: {}", block_hdr.id_); 
    }

    spdlog::info("pull pending block headers: {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::GetBlocks()
{
    GetBlocksRequest request; 
    GetBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->GetBlocks(
        &context, 
        &request, 
        &response,
        [&mu, &cv, &done, &status](grpc::Status s) 
        {
            status = std::move(s);
            std::lock_guard<std::mutex> lock(mu);
            done = true;
            cv.notify_one();
        }
    );

    std::unique_lock<std::mutex> lock(mu);
    while (!done) {
      cv.wait(lock);
    }

    auto resp_blk = response.pb();
    for (auto iter = resp_blk.begin(); iter != resp_blk.end(); iter++)
    {
        std::istringstream iss(*iter);
        Block block; 
        iss >> bits(block);

        spdlog::info("get block: {}, {}", block.header_.id_, block.header_.base_id_); 
    }
    

    spdlog::info("get blocks: {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status; 
}

void TcClient::init()
{
    this->client_id = 0; 
    this->skey = std::make_shared<ecdsa::Key>(ecdsa::Key());
    this->pkey = std::make_shared<ecdsa::PubKey>(this->skey->CreatePubKey());
}

void TcClient::start() 
{
    this->Register(); 
    this->schedule();
}


void TcClient::schedule()
{
    Timer t;

    t.setInterval([&]() {
        this->Heartbeat(); 
    }, (*::conf_data)["heartbeat-interval"]); 

    t.setInterval([&]() {
        this->PullPendingBlocks(); 
    }, (*::conf_data)["pull-pb-interval"]);

    while(true) { sleep(INT_MAX); }
}

}

int main(const int argc, const char* argv[])
{
    spdlog::info("TomChain client starts. "); 

    // set CLI argument parser
    argparse::ArgumentParser parser("tc-client");
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

    // start client
    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            (*::conf_data)["grpc-server-addr"], 
            grpc::InsecureChannelCredentials()
        )
    );
    tcClient.start(); 

    // watch dog
    while(true) { 
        sleep(2);
        spdlog::info("client watchdog");
    }
    return 0;
}