#include <condition_variable>
#include <mutex>
#include <memory>

#include "client/tc-client.hpp"

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"

namespace tomchain {

TcClient::TcClient(std::shared_ptr<grpc::Channel> channel)
    :stub_(TcConsensus::NewStub(channel)) 
{
    this->init(); 
};

RegisterResponse TcClient::Register()
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

    response.set_status(status.ok());

    spdlog::info("register: {}:{}", 
        status.error_code(), 
        status.error_message()
    ); 

    return response;
}

void TcClient::init()
{
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
        spdlog::info("test msg"); 
    }, 1000); 
}

}

int main(const int argc, const char* argv[])
{
    spdlog::info("TomChain client starts. "); 

    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            "localhost:2510", 
            grpc::InsecureChannelCredentials()
        )
    );
    tcClient.start(); 

    while(true) { 
        sleep(2);
        spdlog::info("client watchdog");
    }
    return 0;
}