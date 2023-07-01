#ifndef TC_TRNASACTION_HDR
#define TC_TRNASACTION_HDR

#include <istream>
#include <ostream>

#include "c_plus_plus_serializer.h"
#include "msgpack.hpp"
#include <evmc/evmc.hpp>

namespace tomchain {

/**
 * @brief Define the data structure of Transaction in TomChain. 
 * 
 */
class Transaction {
public: 
    Transaction(); 
    Transaction(
        uint64_t id, 
        uint64_t sender, 
        uint64_t receiver, 
        uint64_t value, 
        uint64_t fee);
    virtual ~Transaction();

// public: 
//     friend std::ostream& operator<<(std::ostream &out, Bits<class Transaction & > obj)
//     {
//         out << 
//             bits(obj.t.id_) << 
//             bits(obj.t.sender_) <<
//             bits(obj.t.receiver_) <<
//             bits(obj.t.value_) << 
//             bits(obj.t.fee_);
//         return (out);
//     }

//     friend std::istream& operator>>(std::istream &in, Bits<class Transaction &> obj)
//     {
//         in >> 
//             bits(obj.t.id_) >> 
//             bits(obj.t.sender_) >>
//             bits(obj.t.receiver_) >>
//             bits(obj.t.value_) >> 
//             bits(obj.t.fee_);
//         return (in);
//     }

public: 
    /**
     * @brief Transaction ID
     * 
     */
    uint64_t id_;

    /**
     * @brief Transaction sender address
     * 
     */
    uint64_t sender_; 

    /**
     * @brief Transaction receiver address
     * 
     */
    uint64_t receiver_; 

    /**
     * @brief Transaction value
     * 
     */
    uint64_t value_;
    
    /**
     * @brief Transaction fee
     * 
     */
    uint64_t fee_; 

    MSGPACK_DEFINE(
        id_, 
        sender_, 
        receiver_, 
        value_, 
        fee_
    ); 
};

}

#endif /* TC_TRNASACTION_HDR */