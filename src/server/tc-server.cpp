#include <thread>
#include <fstream>
#include <random>
#include <chrono>

#include "timercpp/timercpp.h"
#include "spdlog/spdlog.h"
#include "argparse/argparse.hpp"
#include <nlohmann/json.hpp>

std::shared_ptr<nlohmann::json> conf_data;

#include "server/tc-server.hpp"

// gRPC implementations
#include "server/tc-server-grpc.hpp"
#include "server/tc-server-peer-grpc.hpp"

// flatbuffers impl
#include "server/tc-server-grpc-fb.hpp"

namespace tomchain
{

    TcServer::TcServer()
    {
    }

    TcServer::~TcServer()
    {
    }

    void TcServer::init_server()
    {
        spdlog::info("Initializing server");
        this->server_id = (*::conf_data)["server-id"];
        this->blk_seq_generator = (*::conf_data)["server-id"].template get<uint64_t>() * 1000000UL; 
    }

    void TcServer::init_peer_stubs()
    {
        spdlog::info("Initializing peer stubs");
        const uint64_t server_count = (*::conf_data)["server-count"];
        relay_votes.clear();
        for (uint64_t i = 0; i < server_count; i++)
        {
            // server id starts from one
            const uint64_t server_id = i + 1;
            if (server_id == this->server_id)
            {
                continue;
            }

            relay_votes.insert(
                std::make_pair(
                    server_id,
                    std::make_shared<oneapi::tbb::concurrent_queue<
                        std::shared_ptr<BlockVote>>>()));
        }

        relay_blocks.clear();
        for (uint64_t i = 0; i < server_count; i++)
        {
            // server id starts from one
            const uint64_t server_id = i + 1;
            if (server_id == this->server_id)
            {
                continue;
            }

            relay_blocks.insert(
                std::make_pair(
                    server_id,
                    std::make_shared<oneapi::tbb::concurrent_queue<
                        std::shared_ptr<Block>>>()));
        }

        bcast_commit_blocks.clear();
        for (uint64_t i = 0; i < server_count; i++)
        {
            // server id starts from one
            const uint64_t server_id = i + 1;
            if (server_id == this->server_id)
            {
                continue;
            }

            bcast_commit_blocks.insert(
                std::make_pair(
                    server_id,
                    std::make_shared<oneapi::tbb::concurrent_queue<
                        std::shared_ptr<Block>>>()));
        }

        std::vector<std::string> peer_addr = (*::conf_data)["peer-addr"];
        for (size_t i = 0; i < peer_addr.size(); i++)
        {
            // server id starts from one
            const uint64_t server_id = i + 1;
            if (server_id == this->server_id)
            {
                continue;
            }

            grpc_peer_client_stub_.insert(
                std::make_pair(
                    server_id,
                    TcPeerConsensus::NewStub(
                        grpc::CreateChannel(
                            peer_addr.at(i),
                            grpc::InsecureChannelCredentials()))));
        }
    }

    void TcServer::start()
    {
        this->init_server();
        this->init_client_profile();

        // start gRPC server thread
        spdlog::info("Starting gRPC server thread");
        std::thread grpc_thread([&]()
                                {
        std::shared_ptr<TcServer> shared_from_tc_server = 
            shared_from_this(); 
        std::shared_ptr<TcConsensusImpl> consensus_service = 
            std::make_shared<TcConsensusImpl>(); 
        consensus_service->tc_server_ = shared_from_tc_server;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            (*::conf_data)["grpc-listen-addr"], 
            grpc::InsecureServerCredentials()
        ); 
        builder.RegisterService(consensus_service.get());

        grpc_server_ = builder.BuildAndStart();
        grpc_server_->Wait(); });
        grpc_thread.detach();

        // start gRPC peer server thread
        spdlog::info("Starting gRPC peer server thread");
        std::thread grpc_peer_thread([&]()
                                     {
        std::shared_ptr<TcServer> shared_from_tc_server = 
            shared_from_this(); 
        std::shared_ptr<TcPeerConsensusImpl> consensus_service = 
            std::make_shared<TcPeerConsensusImpl>(); 
        consensus_service->tc_server_ = shared_from_tc_server;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            (*::conf_data)["grpc-peer-listen-addr"], 
            grpc::InsecureServerCredentials()
        ); 
        builder.RegisterService(consensus_service.get());

        grpc_peer_server_ = builder.BuildAndStart();
        grpc_peer_server_->Wait(); });
        grpc_peer_thread.detach();

        // Wait all servers to get online 
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        this->init_peer_stubs();

        // start schedule thread
        spdlog::info("Starting schedule thread");
        std::thread schedule_thread([&]
                                    { this->schedule(); });
        schedule_thread.detach();
    }

    void TcServer::init_client_profile()
    {
        spdlog::info("Initializing client profile");

        auto client_count =
            (*::conf_data)["client-count"];

        DKGBLSWrapper dkg_obj(client_count, client_count);
        std::shared_ptr<std::vector<libff::alt_bn128_Fr>> sshares =
            dkg_obj.createDKGSecretShares();

        // client number starts from one
        for (size_t i = 0; i < client_count; i++)
        {
            ClientProfile client_profile;
            client_profile.id = i + 1;

            // TSS keys
            BLSPrivateKeyShare skey_share(
                sshares->at(i),
                client_count,
                client_count);
            BLSPublicKeyShare pkey_share(
                sshares->at(i),
                client_count,
                client_count);
            client_profile.tss_key = std::make_shared<std::pair<
                std::shared_ptr<BLSPrivateKeyShare>,
                std::shared_ptr<BLSPublicKeyShare>>>(
                std::make_pair<
                    std::shared_ptr<BLSPrivateKeyShare>,
                    std::shared_ptr<BLSPublicKeyShare>>(
                    std::make_shared<BLSPrivateKeyShare>(skey_share),
                    std::make_shared<BLSPublicKeyShare>(
                        pkey_share)));

            clients.insert(
                std::make_pair(
                    client_profile.id,
                    std::make_shared<ClientProfile>(
                        client_profile)));
        }
    }

    void TcServer::schedule()
    {
        Timer t;

        // generate transactions
        bool gen_flag = false;
        t.setInterval(
            [&]() {
                if (gen_flag == true) { return; }
                gen_flag = true; 
                // number of generated transactions per second 
                const uint64_t gen_tx_rate = (*::conf_data)["generate-tx-rate"]; 
        
                if (pending_blks.size() < (*::conf_data)["pb-pool-limit"]) 
                {
                    this->generate_tx(gen_tx_rate);
                }
                gen_flag = false; 
            },
            (*::conf_data)["scheduler_freq"]
        );

        // pack blocks
        bool pack_flag = false;
        t.setInterval(
            [&]() {
                if (pack_flag == true) { return; }
                pack_flag = true; 
                // number of generated transactions per second 
                const uint64_t tx_per_block = (*::conf_data)["tx-per-block"]; 
                this->pack_block(tx_per_block, INT_MAX); 
                pack_flag = false; 
            },
            (*::conf_data)["scheduler_freq"]
        );

        // count blocks
        bool count_flag = false;
        t.setInterval(
            [&]() { 
                if (count_flag == true) { return; }
                count_flag = true; 
                spdlog::info(
                    "tx:{} | pb:{} | cb:{}",
                    pending_txs.size(),
                    pending_blks.size(),
                    committed_blks.size()); count_flag = false; 
            },
            (*::conf_data)["scheduler_freq"]
        );

        // peer relay vote
        bool relay_vote_flag = false;
        t.setInterval(
            [&]() { 
                if (relay_vote_flag == true) { return; }
                relay_vote_flag = true;
                this->send_heartbeats(); 
                // this->send_relay_blocks();
                this->send_relay_votes();
                // this->bcast_commits(); 
                relay_vote_flag = false;
            },
            (*::conf_data)["scheduler_freq"]
        ); 

        // peer relay block 
        bool relay_block_flag = false; 
        t.setInterval(
            [&]() { 
                if (relay_block_flag == true) { return; }
                relay_block_flag = true;
                this->send_relay_blocks();
                relay_block_flag = false;
            },
            (*::conf_data)["scheduler_freq"]
        ); 

        // peer bcast commit 
        bool bcast_commit_flag = false; 
        t.setInterval(
            [&]() { 
                if (bcast_commit_flag == true) { return; }
                bcast_commit_flag = true;
                this->bcast_commits(); 
                bcast_commit_flag = false;
            },
            (*::conf_data)["scheduler_freq"]
        ); 

        // merge votes
        bool merge_flag = false;
        t.setInterval(
            [&]() { 
                if (merge_flag == true) { return; }
                merge_flag = true; 
                spdlog::trace("merge_votes thread"); 
                this->merge_votes();
                merge_flag = false;
            },
            (*::conf_data)["scheduler_freq"]
        );

        // TODO: change to shutdown conditional variable
        while (true)
        {
            sleep(INT_MAX);
        }
    }

    void TcServer::merge_votes()
    {
        spdlog::trace("merge_votes starts "); 

        std::shared_ptr<Block> sp_block; 
        while (pb_merge_queue.try_pop(sp_block))
        {
            sp_block->merge_votes((*::conf_data)["client-count"]);

            // get latency in milliseconds 
            uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); 
            uint64_t latency = now_ms - sp_block->header_.proposal_ts_; 
            spdlog::debug("LocalCommit blockid={}, latency={}", sp_block->header_.id_, latency); 

            // record commit timestamp 
            sp_block->header_.commit_ts_ = now_ms; 

            // record recv timestamp 
            sp_block->header_.recv_ts_ = now_ms; 

            // print committed block info in log 
            spdlog::debug("LocalCommit block={}, proposal_ts={}, dist_ts={}, commit_ts={}, recv_ts={}", 
                sp_block->header_.id_, 
                sp_block->header_.proposal_ts_, 
                sp_block->header_.dist_ts_, 
                sp_block->header_.commit_ts_, 
                sp_block->header_.recv_ts_); 

            // insert block to committed
            BlockCHM::accessor cb_accessor;
                this->committed_blks.insert(
                    cb_accessor,
                    sp_block->header_.id_);
                cb_accessor->second = sp_block;

            // insert block to bcast commit
            for (
                auto iter = this->bcast_commit_blocks.begin(); 
                iter != this->bcast_commit_blocks.end(); 
                iter++
            ) {
                iter->second->push(cb_accessor->second);
            }

            cb_accessor.release();

            // remove block from pending
            spdlog::trace("remove block ({}) from pending", sp_block->header_.id_);
            // this->pending_blks.erase(sp_block->header_.id_);

            // this->bcast_commits();
        }

        spdlog::trace("merge_votes ends "); 
    }

    void TcServer::generate_tx(uint64_t num_tx)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<
            std::mt19937::result_type>
            distribution(1, INT_MAX);

        for (size_t i = 0; i < num_tx; i++)
        {
            uint64_t tx_id = distribution(rng);
            Transaction tx(
                tx_id,
                0xDEADBEEF,
                0xDEADBEEF,
                0xDEADBEEF,
                0xDEADBEEF);
            pending_txs.insert(
                std::make_pair(tx_id, std::make_shared<Transaction>(tx)));
        }
    }

    void TcServer::pack_block(uint64_t num_tx, uint64_t num_block)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<
            std::mt19937::result_type>
            distribution(1, INT_MAX);

        for (size_t i = 0; i < num_block; i++)
        {
            std::size_t pending_txs_size = pending_txs.size();
            if (pending_txs_size >= num_tx)
            {
                std::vector<uint64_t> extracted_tx;
                TransactionCHM::iterator it;
                BlockCHM::accessor accessor;

                // construct new block
                // uint64_t block_id = distribution(rng); 
                uint64_t block_id = this->blk_seq_generator.fetch_add(1, std::memory_order_seq_cst); 
                // TODO: base id
                uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); 
                Block new_block(block_id, 0xDEADBEEF, timestamp);
                for (it = pending_txs.begin(); it != pending_txs.end(); ++it)
                {
                    extracted_tx.push_back(it->first);
                    new_block.tx_vec_.push_back(it->second);
                }
                auto p_block = std::make_shared<Block>(new_block);

                // add to relay blocks list
                for (auto iter = relay_blocks.begin(); iter != relay_blocks.end(); iter++)
                {
                    iter->second->push(p_block);
                }
                // this->send_relay_blocks(); 

                // insert into pending blocks
                pending_blks.insert(
                    accessor,
                    block_id);
                accessor->second = p_block;

                // this->send_relay_block_sync(block_id);

                spdlog::trace("gen block: {}", block_id);

                // remove extracted pending transactions
                for (auto iter = extracted_tx.begin(); iter < extracted_tx.end(); iter++)
                {
                    pending_txs.erase(*iter);
                }
            }
            else
            {
                break;
            }
        }
    }

    void TcServer::send_heartbeats()
    {
        for (uint64_t i = 0; i < (*::conf_data)["server-count"]; i++)
        {
            // server id starts from one
            uint64_t target_server_id = i + 1;
            if (target_server_id == server_id)
            {
                continue;
            }
            SPHeartbeat(target_server_id);
        }
    }

    void TcServer::send_relay_votes()
    {
        for (uint64_t i = 0; i < (*::conf_data)["server-count"]; i++)
        {
            // server id starts from one
            uint64_t target_server_id = i + 1;
            if (target_server_id == server_id)
            {
                continue;
            }
            RelayVote(target_server_id);
        }
    }

    void TcServer::send_relay_blocks()
    {
        for (uint64_t i = 0; i < (*::conf_data)["server-count"]; i++)
        {
            // server id starts from one
            uint64_t target_server_id = i + 1;
            if (target_server_id == server_id)
            {
                continue;
            }
            auto status = RelayBlock(target_server_id);
            if (!status.ok())
            {
                spdlog::error("send relay block error"); 
                exit(1); 
            }
        }

        uint64_t block_id; 
        while (this->pb_sync_queue.try_pop(block_id))
        {
            this->send_relay_block_sync(block_id); 
        }
    }

    void TcServer::bcast_commits()
    {
        for (uint64_t i = 0; i < (*::conf_data)["server-count"]; i++)
        {
            // server id starts from one
            uint64_t target_server_id = i + 1;
            if (target_server_id == server_id)
            {
                continue;
            }
            SPBcastCommit(target_server_id);
        }
    }

    void TcServer::send_relay_block_sync(uint64_t block_id)
    {
        for (uint64_t i = 0; i < (*::conf_data)["server-count"]; i++)
        {
            // server id starts from one
            uint64_t target_server_id = i + 1;
            if (target_server_id == server_id)
            {
                continue;
            }
            auto status = RelayBlockSync(block_id, target_server_id);
            if (!status.ok())
            {
                spdlog::error("send relay block sync error"); 
                exit(1); 
            }
        }

        this->pb_sync_labels.insert(block_id);
        spdlog::trace("block ({}) signaled locally", block_id);
    }

}

int main(const int argc, const char *argv[])
{
    spdlog::info("TomChain server starts. ");
    // spdlog::flush_every(std::chrono::seconds(3));

    // set CLI argument parser
    spdlog::trace("Parsing CLI arguments: argc={}", argc);
    for (int i = 0; i < argc; i++)
    {
        spdlog::trace("argv[{}]={}", i, argv[i]);
    }
    argparse::ArgumentParser parser("tc-server");
    parser.add_argument("--cf")
        .help("configuration file")
        .required()
        .default_value(std::string{""});
    parser.add_argument("--id")
        .help("server id")
        .scan<'u', uint32_t>()
        .default_value(1);
    parser.parse_args(argc, argv);

    // parse json configuration
    spdlog::info("Parsing JSON configuration file. ");
    std::string conf_file_path = parser.get<std::string>(
        "--cf");
    std::ifstream fs(conf_file_path);
    ::conf_data = std::make_shared<nlohmann::json>(
        nlohmann::json::parse(fs));

    uint64_t conf_server_id = parser.get<uint32_t>("--id");
    if (conf_server_id != 0)
    {
        (*::conf_data)["server-id"] = conf_server_id;
    }

    spdlog::flush_on(spdlog::level::from_str(
            (*::conf_data)["log-level"])); 
    // set log level
    spdlog::info("Setting log level. ");
    spdlog::set_level(
        spdlog::level::from_str(
            (*::conf_data)["log-level"]));

    // start profiler
    spdlog::info("Starting profiler");
    Timer t;
    if ((*::conf_data)["profiler-enable"])
    {
        EASY_PROFILER_ENABLE;
        t.setTimeout(
            [&]() { 
                std::string filename = 
                    std::string{"profile-server-"} + 
                    std::to_string((*::conf_data)["server-id"].template get<uint64_t>()) + 
                    std::string{".prof"}; 
                profiler::dumpBlocksToFile(filename.c_str()); 
            },
            20000
        );
    }
    if ((*::conf_data)["profiler-listen"])
    {
        profiler::startListen();
    }

    // start server
    spdlog::info("Starting server. ");
    std::shared_ptr<tomchain::TcServer> server =
        std::make_shared<tomchain::TcServer>();
    server->start();

    // watch dog
    while (true)
    {
        sleep(2);
        spdlog::info("server watchdog");
    }
    return 0;
}
