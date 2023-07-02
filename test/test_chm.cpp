#include "block.hpp"
#include "msgpack_packer.hpp"
#include "spdlog/spdlog.h"

#include <thread>
#include "timercpp/timercpp.h"

typedef oneapi::tbb::concurrent_hash_map<
    uint64_t, std::shared_ptr<tomchain::Block>
> BlockCHM; 

int main() {
    BlockCHM chm; 
    BlockCHM::accessor acc;

    Timer t;

    // generate blocks
    t.setInterval([&]() {
        auto chm1 = chm; 
        uint64_t count = 0;
        std::shared_ptr<tomchain::Block> block = 
            std::make_shared<tomchain::Block>(1, 1);
        chm1.insert(acc, count++);
        acc->second = block;
        acc.release(); 
        spdlog::info("add thread: {}", chm1.size());
    }, 1000); 

    sleep(1);

    // delete blocks
    t.setInterval([&]() {
        auto chm2 = chm; 
        uint64_t count = 0;
        auto is_found = chm2.find(acc, count++);
        spdlog::info("is_found: {}", is_found); 
        chm2.erase(acc); 
        acc.release(); 
        spdlog::info("del thread: {}", chm2.size());
    }, 1000); 

    while(true) { sleep(INT_MAX); }

    return 0; 
}