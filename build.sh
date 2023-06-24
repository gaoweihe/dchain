mkdir -p build && cd build
cmake -DCMAKE_PREFIX_PATH=$gRPC_INSTALL_DIR \
    -G Ninja .. \
    -DCMAKE_BUILD_TYPE=Debug 

cmake --build .
cd ..
