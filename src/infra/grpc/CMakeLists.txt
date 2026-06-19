add_library(cs_grpc STATIC
    ChatConPool.h
    ChatConPool.cpp
    ChatGrpcClientImpl.h
    ChatGrpcClientImpl.cpp
    ChatGrpcServiceImpl.h
    ChatGrpcServiceImpl.cpp
    StatusConPool.h
    StatusConPool.cpp
    StatusGrpcClientImpl.h
    StatusGrpcClientImpl.cpp
)

target_link_libraries(cs_grpc PUBLIC
    chatserver_includes
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

add_dependencies(cs_grpc chat_proto_gen)
