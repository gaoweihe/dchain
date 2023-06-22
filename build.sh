export gRPC_INSTALL_DIR=$HOME/.local
export PATH="$gRPC_INSTALL_DIR/bin:$PATH"
# export protobuf_HEADER_DIR=$gRPC_INSTALL_DIR/include/google/protobuf/
# export PATH="$protobuf_HEADER_DIR:$PATH"

protoc -I ./grpc_proto/ --grpc_out=./grpc_proto/ --plugin=protoc-gen-grpc=$gRPC_INSTALL_DIR/bin/grpc_cpp_plugin ./grpc_proto/tc-server.proto
protoc -I ./grpc_proto/ --cpp_out=./grpc_proto/ ./grpc_proto/tc-server.proto
protoc -I ./grpc_proto/ --grpc_out=./grpc_proto/ --plugin=protoc-gen-grpc=$gRPC_INSTALL_DIR/bin/grpc_cpp_plugin ./grpc_proto/tc-server-peer.proto
protoc -I ./grpc_proto/ --cpp_out=./grpc_proto/ ./grpc_proto/tc-server-peer.proto

mkdir -p build && cd build
cmake -DCMAKE_PREFIX_PATH=$gRPC_INSTALL_DIR \
    -G Ninja .. \
    -DCMAKE_BUILD_TYPE=Debug 
cmake --build .
cd ..
