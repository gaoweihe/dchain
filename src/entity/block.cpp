#include "block.hpp"
#include "msgpack_adapter.hpp"

#include <nlohmann/json.hpp>
#include <easy/profiler.h>

extern std::shared_ptr<nlohmann::json> conf_data;

namespace tomchain
{

    Block::Block() {}

    Block::Block(uint64_t id, uint64_t base_id, uint64_t proposal_ts)
        : header_({id, base_id, proposal_ts})
    {
        this->tx_vec_ = std::vector<
            std::shared_ptr<Transaction>>();
    }

    Block::Block(const Block& block)
    {
        this->header_ = block.header_;
        this->tx_vec_ = block.tx_vec_;
        this->tss_sig_ = block.tss_sig_; 
        this->votes_ = block.votes_; 
    }

    Block::~Block()
    {
    }

    void Block::insert(std::shared_ptr<Transaction> tx)
    {
        this->tx_vec_.push_back(tx);
    }

    std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> Block::get_sha256()
    {
        EASY_FUNCTION("get_sha256");
         
        std::vector<unsigned char> hash(picosha2::k_digest_size);

        msgpack::sbuffer b;
        msgpack::pack(b, *this);
        std::string ss_str = sbufferToString(b);

        picosha2::hash256(ss_str.begin(), ss_str.end(), hash.begin(), hash.end());

        std::array<uint8_t, picosha2::k_digest_size> hash_arr;
        std::copy_n(hash.begin(), picosha2::k_digest_size, hash_arr.begin());

        std::shared_ptr<std::array<uint8_t, picosha2::k_digest_size>> spHashArr =
            std::make_shared<std::array<uint8_t, picosha2::k_digest_size>>(hash_arr);
        return spHashArr;
    }

    std::set<uint64_t> Block::get_server_id(uint64_t server_count) const
    {
        uint64_t primary_id = (this->header_.id_ % server_count) + 1;
        uint64_t shadow_id = primary_id + 1; 
        if (shadow_id > server_count)
        {
            shadow_id = 1; 
        }
        std::set<uint64_t> server_id_list; 
        server_id_list.insert(primary_id); 
        server_id_list.insert(shadow_id); 
        return server_id_list;
    }

    bool Block::is_vote_enough(const uint64_t target_num) const
    {
        spdlog::trace("{}: Block::is_vote_enough()", target_num); 
        return votes_.size() >= target_num;
    }

    void Block::merge_votes(const uint64_t target_num)
    {
        EASY_FUNCTION("merge_votes");
        spdlog::trace("{}: merge votes", target_num); 

        BLSSigShareSet sig_share_set(
            target_num,
            target_num);

        // unsafe iterations on concurrent hash map
        // but it is locked by pb_accessor
        spdlog::trace("{}: iterate chm", target_num); 
        for (
            auto vote_iter = votes_.begin();
            vote_iter != votes_.end();
            vote_iter++)
        {
            sig_share_set.addSigShare(
                vote_iter->second->sig_share_);
        }

        spdlog::trace("{}: check sig enough", target_num); 
        if (sig_share_set.isEnough())
        {
            // merge signature
            // size_t merge_threads = (*::conf_data)["merge-threads"]; 
            spdlog::trace("{}: merge sigset", target_num); 
            std::shared_ptr<BLSSignature> tss_sig = sig_share_set.merge(16);
            spdlog::trace("{}: merge complete", target_num); 
            this->tss_sig_ = tss_sig;
        }
    }

    BlockVote::BlockVote() {}

    BlockVote::BlockVote(const BlockVote& bv)
    {
        this->block_id_ = bv.block_id_;
        this->voter_id_ = bv.voter_id_;
        this->sig_share_ = bv.sig_share_;
    }

    BlockHeader::BlockHeader() {}

    BlockHeader::BlockHeader(uint64_t id, uint64_t base_id, uint64_t proposal_ts)
        : id_(id), base_id_(base_id), proposal_ts_(proposal_ts)
    {
    }

    BlockHeader::BlockHeader(const BlockHeader& bh)
    {
        this->id_ = bh.id_;
        this->base_id_ = bh.base_id_;
        this->proposal_ts_ = bh.proposal_ts_;  
        this->dist_ts_ = bh.dist_ts_;
        this->commit_ts_ = bh.commit_ts_;
        this->recv_ts_ = bh.recv_ts_; 
    }

}