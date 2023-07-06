#include "spdlog/spdlog.h"

#include <grpcpp/grpcpp.h>
#include "tc-server-peer.grpc.pb.h"

class TcPeerConsensusImpl final : public TcPeerConsensus::CallbackService
{

public:
    std::shared_ptr<TcServer> tc_server_;

public:
    /**
     * @brief Client registers when it connects to server.
     *
     * @param context RPC context.
     * @param request RPC request.
     * @param response RPC response.
     * @return grpc::Status RPC status.
     */
    grpc::ServerUnaryReactor *RelayVote(
        grpc::CallbackServerContext *context,
        const RelayVoteRequest *request,
        RelayVoteResponse *response) override
    {
        spdlog::debug("gRPC(RelayVote) starts"); 

        uint32_t peer_id = request->id(); 
        auto req_votes = request->votes(); 

        for (auto iter = req_votes.begin(); iter != req_votes.end(); iter++)
        {
            // deserialize relayed votes 
        }

        grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }
};