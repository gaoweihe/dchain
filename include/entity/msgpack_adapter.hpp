#pragma once
#ifndef TC_MSGPACK_ADAPTER
#define TC_MSGPACK_ADAPTER

#include "block.hpp"
#include "libBLS.h"
#include <evmc/evmc.hpp>
#include "spdlog/spdlog.h"

namespace tomchain {

static std::string sbufferToString(const msgpack::sbuffer& sbuf) {
    std::string str(sbuf.data(), sbuf.size());
    return str;
}

static msgpack::sbuffer stringToSbuffer(const std::string& str) {
    msgpack::object_handle oh;
    msgpack::unpack(oh, str.data(), str.size());
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, oh.get());
    return sbuf;
}

};

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

// BLSSigShare
template <>
struct pack<BLSSigShare> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, BLSSigShare const& v) const {
        o.pack_map(4);

        auto sig_share = v.getSigShare();
        auto hint = v.getHint();
        auto signer_index = v.getSignerIndex();
        auto t = v.getRequiredSigners();
        auto n = v.getTotalSigners();

        BLSSigShare tmp_sig_share(
            sig_share,
            hint,
            signer_index,
            t,
            n
        ); 

        o.pack("sig_share_str");
        o.pack(tmp_sig_share.toString());

        o.pack("signer_index");
        o.pack(tmp_sig_share.getSignerIndex());

        o.pack("t");
        o.pack(tmp_sig_share.getRequiredSigners());

        o.pack("n");
        o.pack(tmp_sig_share.getTotalSigners());

        return o;
    }
};

template <>
struct as<BLSSigShare> {
    BLSSigShare operator()(msgpack::object const& o) const {
        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 4) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        BLSSigShare sig_share(
            std::make_shared<std::string>(m["sig_share_str"].as<std::string>()), 
            m["signer_index"].as<uint64_t>(),
            m["t"].as<uint64_t>(),
            m["n"].as<uint64_t>()
        );

        return sig_share; 
    }
};

// BLSSignature
template <>
struct pack<BLSSignature> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, const BLSSignature& v) const {
        o.pack_map(3);

        auto sig = v.getSig();
        auto hint = v.getHint();
        auto t = v.getRequiredSigners();
        auto n = v.getTotalSigners();
        BLSSignature tmp_sig(
            sig,
            hint,
            t,
            n
        );

        o.pack("sig_str");
        o.pack(tmp_sig.toString());

        o.pack("t");
        o.pack(tmp_sig.getRequiredSigners());

        o.pack("n");
        o.pack(tmp_sig.getTotalSigners());

        return o;
    }
};

template <>
struct as<BLSSignature> {
    BLSSignature operator()(msgpack::object const& o) const {
        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 3) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        BLSSignature signature(
            std::make_shared<std::string>(m["sig_str"].as<std::string>()), 
            m["t"].as<uint64_t>(),
            m["n"].as<uint64_t>()
        );

        return signature; 
    }
};

//evmc::address
template <>
struct pack<evmc::address> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, evmc::address& v) const {
        o.pack_map(1);

        o.pack("addr_bytes");
        o.pack(
            std::string(
                reinterpret_cast<const char*>(v.bytes), 
                sizeof(v.bytes)
            )
        ); 

        return o;
    }
};

template <>
struct as<evmc::address> {
    evmc::address operator()(msgpack::object const& o) const {
        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 1) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        evmc::address addr;
        // copy bytes to addr 
        std::string bytes_str = m["addr_bytes"].as<std::string>();
        std::memcpy(addr.bytes, bytes_str.c_str(), sizeof(addr.bytes));

        return addr; 
    }
};

template <>
struct pack<tomchain::BlockVote> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, tomchain::BlockVote const& v) const {
        o.pack_map(3);

        o.pack("block_id"); 
        o.pack(v.block_id_);

        o.pack("voter_id");
        o.pack(v.voter_id_);

        o.pack("sig_share");
        o.pack(v.sig_share_); 

        return o;
    }
};

template <>
struct as<tomchain::BlockVote> {
    tomchain::BlockVote operator()(msgpack::object const& o) const {
        tomchain::BlockVote bv; 

        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 3) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        bv.block_id_ = m["block_id"].as<uint64_t>();
        bv.sig_share_ = m["sig_share"].as<std::shared_ptr<BLSSigShare>>();
        bv.voter_id_ = m["voter_id"].as<uint64_t>();

        return bv; 
    }
};

template <>
struct pack<tomchain::Block> {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, tomchain::Block const& v) const {
        o.pack_map(4);

        o.pack("header"); 
        o.pack(v.header_);

        o.pack("tx_vec");
        o.pack(v.tx_vec_); 

        o.pack("votes");
        o.pack(v.votes_); 

        o.pack("tss_sig");
        o.pack(v.tss_sig_); 

        return o;
    }
};

template <>
struct as<tomchain::Block> {
    tomchain::Block operator()(msgpack::object const& o) const {
        tomchain::Block block; 

        if (o.type != msgpack::type::MAP) throw msgpack::type_error();
        if (o.via.map.size != 4) throw msgpack::type_error();
        std::map<std::string, msgpack::object> m;
        o >> m;

        block.header_ = m["header"].as<tomchain::BlockHeader>();
        block.tx_vec_ = m["tx_vec"].as<
            std::vector<std::shared_ptr<tomchain::Transaction>>
        >();
        block.votes_ = m["votes"].as<
            std::map<uint64_t, std::shared_ptr<tomchain::BlockVote>>
        >(); 
        block.tss_sig_ = m["tss_sig"].as<std::shared_ptr<BLSSignature>>();

        return block; 
    }
};


} // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

#endif /* TC_MSGPACK_ADAPTER */
