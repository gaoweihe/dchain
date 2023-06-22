#include "client/tc-client.hpp"

int main(const int argc, const char* argv[])
{
    tomchain::TcClient tcClient(
        grpc::CreateChannel("", grpc::InsecureChannelCredentials())
    ); 
    return 0; 
}