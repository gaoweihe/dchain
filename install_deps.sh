rm -rf ./third_party
mkdir third_party && cd third_party 

# gRPC
export gRPC_INSTALL_DIR=$HOME/bin/grpc/
git clone --recurse-submodules -b v1.55.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$gRPC_INSTALL_DIR \
      ../..
make -j 4
make install
popd
cd .. 

# evmc 
git clone -b v10.1.0 https://github.com/ethereum/evmc.git

cd ..
