add_library(cs_grpc STATIC
    ChatGrpcClient.cpp
    ChatConPool.cpp
    StatusGrpcClient.cpp
    StatusConPool.cpp
)
add_dependencies(cs_grpc chat_proto_gen)
target_link_libraries(cs_grpc PUBLIC chatserver_includes gRPC::grpc++ protobuf::libprotobuf cs_config cs_common)
