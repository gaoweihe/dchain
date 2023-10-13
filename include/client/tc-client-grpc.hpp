#include <grpcpp/grpcpp.h>
#include "tc-server.grpc.pb.h"

#include "flexbuffers_adapter.hpp"

namespace tomchain
{

    grpc::Status TcClient::Register(uint64_t stub_id)
    {
        RegisterRequest request;
        request.set_id(this->client_id);
        const std::vector<uint8_t> &ser_pkey = this->ecc_pkey->get_pub_key_data();
        request.set_pkey(ser_pkey.data(), ser_pkey.size());
        RegisterResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        grpc::Status status;
        stubs.at(stub_id)->async()->Register(
            &context,
            &request,
            &response,
            [&mu, &cv, &done, &status](grpc::Status s)
            {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one();
            });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }

        auto tss_sk_str = response.tss_sk();
        BLSPrivateKeyShare skey_share(
            tss_sk_str,
            (*::conf_data)["client-count"],
            (*::conf_data)["client-count"]);
        std::shared_ptr<libff::alt_bn128_Fr> skey_raw = skey_share.getPrivateKey();
        BLSPublicKeyShare pkey_share(
            *skey_raw,
            (*::conf_data)["client-count"],
            (*::conf_data)["client-count"]);
        this->tss_key = std::make_shared<std::pair<
            std::shared_ptr<BLSPrivateKeyShare>,
            std::shared_ptr<BLSPublicKeyShare>>>(
            std::make_pair<
                std::shared_ptr<BLSPrivateKeyShare>,
                std::shared_ptr<BLSPublicKeyShare>>(
                std::make_shared<BLSPrivateKeyShare>(skey_share),
                std::make_shared<BLSPublicKeyShare>(pkey_share)));

        spdlog::trace("gRPC(Register): {}:{}",
                      status.error_code(),
                      status.error_message());

        return status;
    }

    grpc::Status TcClient::Heartbeat(uint64_t stub_id)
    {
        HeartbeatRequest request;
        request.set_id(this->client_id);
        HeartbeatResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        grpc::Status status;
        stubs.at(stub_id)->async()->Heartbeat(
            &context,
            &request,
            &response,
            [&mu, &cv, &done, &status](grpc::Status s)
            {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one();
            });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }

        spdlog::trace("gRPC(Heartbeat): {}:{}",
                      status.error_code(),
                      status.error_message());

        return status;
    }

    grpc::Status TcClient::PullPendingBlocks(uint64_t stub_id)
    {
        EASY_BLOCK("PullPendingBlocks_req");
        spdlog::trace("gRPC(PullPendingBlocks) starts");

        PullPendingBlocksRequest request;
        request.set_id(this->client_id);
        PullPendingBlocksResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("waiting");
        spdlog::debug("PullPendingBlocks waiting for stub {}", stub_id);
        grpc::Status status;
        stubs.at(stub_id)->async()->PullPendingBlocks(
            &context,
            &request,
            &response,
            [&mu, &cv, &done, &status](grpc::Status s)
            {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one();
            });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        // unpack response
        EASY_BLOCK("unpack response");
        for (int i = 0; i < response.pb_hdrs_size(); i++)
        {
            auto iter = response.pb_hdrs(i);
            // msgpack::sbuffer des_b = stringToSbuffer(response.pb_hdrs(i));
            // auto oh = msgpack::unpack(des_b.data(), des_b.size());
            // auto block_hdr = oh->as<BlockHeader>();
            std::vector<uint8_t> blkhdr_ser(iter.begin(), iter.end());
            auto block_hdr =
                flexbuffers_adapter<BlockHeader>::from_bytes(
                    std::make_shared<std::vector<uint8_t>>(blkhdr_ser));

            pending_blkhdr.insert(
                std::make_pair(
                    block_hdr->id_,
                    block_hdr));
            spdlog::trace("block id: {}", block_hdr->id_);
        }
        EASY_END_BLOCK;

        // if (response.pb_hdrs_size() == 0)
        // {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(30));
        // }

        spdlog::trace("gRPC(PullPendingBlocks): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

    grpc::Status TcClient::GetBlocks(uint64_t stub_id)
    {
        EASY_BLOCK("GetBlocks_req");
        spdlog::trace("gRPC(GetBlocks): start");

        GetBlocksRequest request;

        EASY_BLOCK("render request");
        for (auto iter = pending_blkhdr.begin(); iter != pending_blkhdr.end(); iter++)
        {
            // msgpack::sbuffer b;
            // msgpack::pack(b, iter->second);
            // std::string blk_hdr_str = sbufferToString(b);
            auto hdr_bv = flexbuffers_adapter<BlockHeader>::to_bytes(*(iter->second));
            std::string blk_hdr_str(hdr_bv->begin(), hdr_bv->end());

            request.add_pb_hdrs(blk_hdr_str);
        }
        EASY_END_BLOCK;

        GetBlocksResponse response;

        grpc::ClientContext context;
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;

        EASY_BLOCK("waiting");
        spdlog::debug("GetBlocks waiting for stub {}", stub_id);
        grpc::Status status;
        stubs.at(stub_id)->async()->GetBlocks(
            &context,
            &request,
            &response,
            [&mu, &cv, &done, &status](grpc::Status s)
            {
                status = std::move(s);
                std::lock_guard<std::mutex> lock(mu);
                done = true;
                cv.notify_one();
            });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }
        EASY_END_BLOCK;

        EASY_BLOCK("unpack response");
        auto resp_blk = response.pb();
        for (auto iter = resp_blk.begin(); iter != resp_blk.end(); iter++)
        {
            EASY_BLOCK("deserialize");
            // msgpack::sbuffer des_b = stringToSbuffer(*iter);
            // auto oh = msgpack::unpack(des_b.data(), des_b.size());
            // auto block = oh->as<Block>();
            std::vector<uint8_t> blk_ser((*iter).begin(), (*iter).end());
            auto block =
                flexbuffers_adapter<Block>::from_bytes(
                    std::make_shared<std::vector<uint8_t>>(blk_ser));
            EASY_END_BLOCK;

            spdlog::info("pb tx count = {}", block->tx_vec_.size()); 

            EASY_BLOCK("insert into pb");
            pending_blks.push(
                block);
            EASY_END_BLOCK;

            // EASY_BLOCK("vote");
            // this->VoteBlocks();
            // EASY_END_BLOCK;

            // remove block header from CHM
            EASY_BLOCK("remove");
            pending_blkhdr.erase(block->header_.id_);
            EASY_END_BLOCK;

            spdlog::trace("get block: {}, {}", block->header_.id_, block->header_.base_id_);
        }
        EASY_END_BLOCK;

        spdlog::trace("gRPC(GetBlocks): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

    grpc::Status TcClient::VoteBlocks()
    {
        EASY_BLOCK("VoteBlocks_req");
        spdlog::trace("gRPC(VoteBlocks): start");

        grpc::Status status;

        std::shared_ptr<Block> sp_block;
        while (pending_blks.try_pop(sp_block))
        {
            VoteBlocksRequest request;
            request.set_id(this->client_id);

            // check transactions
            EASY_BLOCK("check tx");
            std::unique_lock<std::mutex> db_lg_1(db_mutex);
            spdlog::info("tx count: {}", sp_block->tx_vec_.size()); 
            for (auto iter = sp_block->tx_vec_.begin(); iter != sp_block->tx_vec_.end(); iter++)
            {
                const uint64_t sender = (*iter)->sender_;
                const uint64_t receiver = (*iter)->receiver_;

                // retrieve balance of sender from rocksdb
                std::string sender_balance;
                std::string receiver_balance;
                db->Get(rocksdb::ReadOptions(), std::to_string(sender), &sender_balance);
                db->Get(rocksdb::ReadOptions(), std::to_string(receiver), &receiver_balance);

                // update balance of receiver to rocksdb
                db->Put(rocksdb::WriteOptions(), std::to_string(sender), std::to_string(sender));
                db->Put(rocksdb::WriteOptions(), std::to_string(receiver), std::to_string(receiver));
            }
            db_lg_1.unlock();
            EASY_END_BLOCK;

            auto block_hash_str = sp_block->get_sha256();
            const uint64_t block_id = sp_block->header_.id_;

            // client_id starts from 1, so does signer_index
            EASY_BLOCK("sign");
            auto signer_index = this->client_id;
            std::shared_ptr<BLSSigShare> sig_share =
                this->tss_key->first->sign(block_hash_str, this->client_id);
            BlockVote bv;
            bv.block_id_ = sp_block->header_.id_;
            bv.sig_share_ = sig_share;
            bv.voter_id_ = this->client_id;
            EASY_END_BLOCK;

            EASY_BLOCK("insert into votes");
            sp_block->votes_.insert(
                std::make_pair(
                    this->client_id,
                    std::make_shared<BlockVote>(bv)));
            EASY_END_BLOCK;

            EASY_BLOCK("serialize");
            spdlog::trace("gRPC(VoteBlocks): serialize");
            Block block = *(sp_block);
            // msgpack::sbuffer b;
            // msgpack::pack(b, iter->second);
            // std::string block_ser = sbufferToString(b);
            auto block_bv = flexbuffers_adapter<Block>::to_bytes(block);
            std::string block_ser(block_bv->begin(), block_bv->end());
            EASY_END_BLOCK;

            EASY_BLOCK("add voted blocks");
            request.add_voted_blocks(block_ser);
            EASY_END_BLOCK;

            for (uint64_t stub_id = 0; stub_id < 2; stub_id++)
            {
                VoteBlocksResponse response;

                grpc::ClientContext context;
                std::mutex mu;
                std::condition_variable cv;
                bool done = false;

                EASY_BLOCK("waiting");
                spdlog::debug("VoteBlocks waiting for stub {}", stub_id);
                stubs.at(stub_id)->async()->VoteBlocks(
                    &context,
                    &request,
                    &response,
                    [&mu, &cv, &done, &status](grpc::Status s)
                    {
                        status = std::move(s);
                        std::lock_guard<std::mutex> lock(mu);
                        done = true;
                        cv.notify_one();
                    });

                std::unique_lock<std::mutex> lock(mu);
                while (!done)
                {
                    cv.wait(lock);
                }
                lock.unlock();
                EASY_END_BLOCK;

                EASY_BLOCK("unpack response");
                spdlog::trace("gRPC(VoteBlocks): recv response");
                // for (auto iter = pending_blks.begin(); iter != pending_blks.end(); iter++)
                // {
                //     voted_blks.insert(
                //         std::make_pair(
                //             iter->first,
                //             iter->second
                //         )
                //     );
                // }
                // pending_blks.clear();
                EASY_END_BLOCK;
            }
        }

        spdlog::trace("gRPC(VoteBlocks): {}:{}",
                      status.error_code(),
                      status.error_message());

        EASY_END_BLOCK;

        return status;
    }

};
