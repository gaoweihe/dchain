#include "block.hpp"
#include "msgpack_adapter.hpp"

#include <nlohmann/json.hpp>
#include <easy/profiler.h>

extern std::shared_ptr<nlohmann::json> conf_data;

namespace tomchain
{

    Block::Block() {}

    Block::Block(uint64_t id, uint64_t base_id)
        : header_({id, base_id})
    {
        this->tx_vec_ = std::vector<
            std::shared_ptr<Transaction>>();
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

    uint64_t Block::get_server_id(uint64_t server_count) const
    {
        return (this->header_.id_ % server_count) + 1;
    }

    bool Block::is_vote_enough(const uint64_t target_num) const
    {
        return votes_.size() >= target_num;
    }

    void Block::merge_votes(const uint64_t target_num)
    {
        EASY_FUNCTION("merge_votes");

        BLSSigShareSet sig_share_set(
            target_num,
            target_num);

        // unsafe interations on concurrent hash map
        // but it is locked by pb_accessor
        for (
            auto vote_iter = votes_.begin();
            vote_iter != votes_.end();
            vote_iter++)
        {
            sig_share_set.addSigShare(
                vote_iter->second->sig_share_);
        }

        if (sig_share_set.isEnough())
        {
            // merge signature
            std::shared_ptr<BLSSignature> tss_sig = sig_share_set.merge();
            tss_sig_ = tss_sig;
        }
    }

    BlockVote::BlockVote() {}

}