#include <grpcpp/grpcpp.h>
#include "flatbuffers/grpc.h"
#include "consensus_generated.h"
#include "consensus.grpc.fb.h"

int main()
{
    flatbuffers::grpc::MessageBuilder mb; 
    auto str_offset = mb.CreateString("hello"); 
    // auto msg_offset = CreateVotedBlocks
    // mb.ReleaseMessage<VoteBlocksRequest>(); 
    
    return 0; 
}