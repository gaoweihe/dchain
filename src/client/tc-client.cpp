#include <condition_variable>

#include "client/tc-client.hpp"

namespace tomchain {

RegisterResponse TcClient::Register()
{
    RegisterRequest request; 
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
    return response;
}

void TcClient::start() 
{
    
}

}

int main(const int argc, const char* argv[])
{
    tomchain::TcClient tcClient(
        grpc::CreateChannel(
            "localhost:2510", 
            grpc::InsecureChannelCredentials()
        )
    );
    tcClient.start(); 
    tcClient.Register(); 

    return 0;
}