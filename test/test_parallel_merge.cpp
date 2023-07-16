#include "libBLS/libBLS.h"

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

    {
        BLSSigShareSet sig_set(num_signed, num_all); 
        for (size_t i = 0; i < keys->first->size(); i++)
        {
            std::shared_ptr<BLSPrivateKeyShare> skey_share = keys->first->at(i); 
            std::shared_ptr<BLSSigShare> sig_share = skey_share->sign(spHashArr, i + 1); 
            sig_set.addSigShare(sig_share); 
        }
    
        if (sig_set.isEnough())
        {
            serial_sig = sig_set.merge();
        }
    }

    {
        BLSSigShareSet sig_set(num_signed, num_all); 
        for (size_t i = 0; i < keys->first->size(); i++)
        {
            std::shared_ptr<BLSPrivateKeyShare> skey_share = keys->first->at(i); 
            std::shared_ptr<BLSSigShare> sig_share = skey_share->sign(spHashArr, i + 1); 
            sig_set.addSigShare(sig_share); 
        }
    
        if (sig_set.isEnough())
        {
            parallel_sig = sig_set.merge(4);
        }
    }

    assert(*(serial_sig->getSig()) == *(parallel_sig->getSig()));

    assert(libff::alt_bn128_q_limbs == 4); 
    assert(sizeof(libff::alt_bn128_G1) == 96); 

    return 0; 
}