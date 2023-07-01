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
    request.set_id(this->client_id);
    const std::vector<uint8_t>& ser_pkey = this->ecc_pkey->get_pub_key_data();
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

    auto tss_sk_str = response.tss_sk();
    BLSPrivateKeyShare skey_share(
        tss_sk_str, 
        (*::conf_data)["client-count"], 
        (*::conf_data)["client-count"]
    );
    std::shared_ptr<libff::alt_bn128_Fr> skey_raw = skey_share.getPrivateKey(); 
    BLSPublicKeyShare pkey_share(
        *skey_raw, 
        (*::conf_data)["client-count"], 
        (*::conf_data)["client-count"]
    ); 
    this->tss_key = std::make_shared<std::pair<
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

    spdlog::info("gRPC(register): {}:{}", 
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

    spdlog::info("gRPC(heartbeat): {}:{}", 
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
        pending_blkhdr.insert(
            std::make_pair(
                block_hdr.id_, 
                std::make_shared<BlockHeader>(block_hdr)
            )
        );
        spdlog::info("block id: {}", block_hdr.id_); 
    }

    spdlog::info("gRPC(pull pending block headers): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status;
}

grpc::Status TcClient::GetBlocks()
{
    GetBlocksRequest request; 

    for (auto iter = pending_blkhdr.begin(); iter != pending_blkhdr.end(); iter++)
    {
        std::stringstream ss; 
        ss << bits(*(iter->second));
        auto blk_hdr_str = ss.str();
        request.add_pb_hdrs(blk_hdr_str);
    }
    
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

    spdlog::info("stub1"); 

    auto resp_blk = response.pb();
    for (auto iter = resp_blk.begin(); iter != resp_blk.end(); iter++)
    {
        spdlog::info("stub2");

        std::istringstream iss(*iter);
        spdlog::info("stub21: {}", spdlog::to_hex(*iter));
        tomchain::Block block; 
        iss >> bits(block);

        spdlog::info("stub3"); 

        pending_blks.insert(
            std::make_pair(
                block.header_.id_, 
                std::make_shared<tomchain::Block>(block)
            )
        ); 

        spdlog::info("stub4"); 

        // remove block header from CHM
        pending_blkhdr.erase(block.header_.id_); 

        spdlog::info("stub5"); 

        spdlog::info("get block: {}, {}", block.header_.id_, block.header_.base_id_); 
    }

    spdlog::info("stub6"); 
    

    spdlog::info("gRPC(get blocks): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status; 
}

grpc::Status TcClient::VoteBlocks()
{
    VoteBlocksRequest request; 
    request.set_id(this->client_id); 
    for (auto iter = pending_blks.begin(); iter != pending_blks.end(); iter++)
    {
        auto block_hash_str = iter->second->get_sha256();

        // client_id starts from 1, whereas signer_index starts from 0 
        auto signer_index = this->client_id - 1;
        std::shared_ptr<BLSSigShare> sig_share = 
            this->tss_key->first->sign(block_hash_str, this->client_id); 
        BlockVote bv;
        bv.sig_share_ = sig_share;

        const uint64_t block_id = iter->second->header_.id_; 
        iter->second->votes_.insert(
            std::make_pair(
                block_id, 
                std::make_shared<BlockVote>(bv)
            )
        );

        std::stringstream ss; 
        ss << bits(*(iter->second));
        std::string block_ser = ss.str(); 
        request.add_voted_blocks(block_ser); 
    }
    
    VoteBlocksResponse response; 

    grpc::ClientContext context;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;

    grpc::Status status;
    stub_->async()->VoteBlocks(
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

    for (auto iter = pending_blks.begin(); iter != pending_blks.end(); iter++)
    {
        voted_blks.insert(
            std::make_pair(
                iter->first, 
                iter->second
            )
        ); 
    }
    pending_blks.clear(); 
    

    spdlog::info("gRPC(vote blocks): {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return status; 
}

void TcClient::init()
{
    this->client_id = (*::conf_data)["client-id"]; 
    this->ecc_skey = std::make_shared<ecdsa::Key>(ecdsa::Key());
    this->ecc_pkey = std::make_shared<ecdsa::PubKey>(
        this->ecc_skey->CreatePubKey()
    );
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
        this->GetBlocks(); 
        this->VoteBlocks(); 
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