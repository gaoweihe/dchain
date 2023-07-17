#include <grpcpp/grpcpp.h>
// #include "flatbuffers/flatbuffers.h"
#include "consensus.grpc.fb.h"
#include "consensus_generated.h"

#include "libBLS/libBLS.h"

#include <memory>

namespace tomchain
{

    class TcServer;

    class TcConsensusFbImpl final : public fb::Consensus::Service
    {

    public: 

        virtual grpc::Status VoteBlocks(
            grpc::ServerContext *context,
            const flatbuffers::grpc::Message<fb::VoteBlocksRequest> *request_msg,
            flatbuffers::grpc::Message<fb::VoteBlocksResponse> *response_msg) override
        {
            flatbuffers::grpc::MessageBuilder mb_;

            // We call GetRoot to "parse" the message. Verification is already
            // performed by default. See the notes below for more details.
            const fb::VoteBlocksRequest *request = request_msg->GetRoot();

            // // Fields are retrieved as usual with FlatBuffers
            // const std::string &name = request->name()->str();
            const uint32_t &client_id = request->id(); 
            auto votes = request->votes(); 
            for (auto vote : *votes) {
                auto limbs = vote->sigshare()->point()->limbs(); 
                libff::alt_bn128_G1 *g1 = (libff::alt_bn128_G1*)limbs->data();
            }

            // // `flatbuffers::grpc::MessageBuilder` is a `FlatBufferBuilder` with a
            // // special allocator for efficient gRPC buffer transfer, but otherwise
            // // usage is the same as usual.
            // auto msg_offset = mb_.CreateString("Hello, " + name);
            // auto hello_offset = CreateHelloReply(mb_, msg_offset);
            // mb_.Finish(hello_offset);

            // The `ReleaseMessage<T>()` function detaches the message from the
            // builder, so we can transfer the resopnse to gRPC while simultaneously
            // detaching that memory buffer from the builer.
            *response_msg = mb_.ReleaseMessage<fb::VoteBlocksResponse>();
            assert(response_msg->Verify());

            // Return an OK status.
            return grpc::Status::OK;
        }

    public:
        /**
         * @brief Reference to TomChain server instance
         *
         */
        std::shared_ptr<TcServer> tc_server_;
    };
}