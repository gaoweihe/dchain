#include <atomic>

namespace tomchain
{
    class FBBMgr
    {
    public:
        std::atomic<uint64_t> m_fbb_id;

    public: 
        FBBMgr() : m_fbb_id(0) {}
        ~FBBMgr() {}

    public: 
        void add_fbb(std::shared_ptr<> fbb); 
        void tie_block(const uint32_t block_id); 
        void untie_block(const uint32_t block_id);
    };
}