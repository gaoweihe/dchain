#include "flexbuffers_adapter.hpp"

using namespace tomchain; 

int main()
{
    std::shared_ptr<BLSSignature> serial_sig; 
    std::shared_ptr<BLSSignature> parallel_sig; 

    std::array<uint8_t, 32> hash_arr;
    std::shared_ptr<std::array<uint8_t, 32>> spHashArr =
        std::make_shared<std::array<uint8_t, 32>>(hash_arr);

    const size_t num_signed = 100;
    const size_t num_all = 100; 
    std::shared_ptr<std::pair<
        std::shared_ptr<std::vector<std::shared_ptr<BLSPrivateKeyShare>>>, 
        std::shared_ptr<BLSPublicKey>>> keys = 
            BLSPrivateKeyShare::generateSampleKeys(num_signed, num_all);

    std::shared_ptr<BLSPrivateKeyShare> skey_share = keys->first->at(0); 
    std::shared_ptr<BLSSigShare> sig_share = skey_share->sign(spHashArr, 1); 

    std::shared_ptr<std::vector<uint8_t>> bytes_ser = flexbuffers_adapter<BLSSigShare>::to_bytes(*sig_share);
    std::shared_ptr<BLSSigShare> sig_share_des = flexbuffers_adapter<BLSSigShare>::from_bytes(bytes_ser);

    assert(*(sig_share->getSigShare()) == *(sig_share_des->getSigShare()));
    
    return 0; 
}