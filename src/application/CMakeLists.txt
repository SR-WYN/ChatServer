add_library(cs_application STATIC
    LogicSystem.cpp
    ChatServiceImpl.cpp
    UserMgr.cpp
)
add_dependencies(cs_application chat_proto_gen)
target_link_libraries(cs_application PUBLIC
    chatserver_includes
    cs_domain
    cs_net
    cs_common
    cs_config
    cs_redis
    cs_mysql
    cs_grpc
)
