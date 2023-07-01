#include "block.hpp"

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

template <>
struct pack<tomchain::BlockVote> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, tomchain::BlockVote const& v) const {
        o.pack_map(5);

        o.pack("block_id"); 
        o.pack(v.block_id_);

        o.pack("signer_index");
        o.pack(v.sig_share_->getSignerIndex());

        o.pack("t");
        o.pack(v.sig_share_->getRequiredSigners());

        o.pack("n");
        o.pack(v.sig_share_->getTotalSigners());

        o.pack("sig_share_str");
        o.pack(v.sig_share_->toString());

        return o;
    }
};

template <>
struct as<tomchain::BlockVote> {
    tomchain::BlockVote operator()(msgpack::object const& o) const {
        tomchain::BlockVote bv; 

        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 5) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        bv.block_id_ = m["block_id"].as<uint64_t>();
        BLSSigShare sig_share(
            std::make_shared<std::string>(m["sig_share_str"].as<std::string>()), 
            m["signer_index"].as<uint64_t>(),
            m["t"].as<uint64_t>(),
            m["n"].as<uint64_t>()
        );
        bv.sig_share_ = std::make_shared<BLSSigShare>(sig_share);

        return bv; 
    }
};


} // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack