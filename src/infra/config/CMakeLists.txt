add_library(cs_config STATIC
    ConfigMgr.h
    ConfigMgr.cpp
    RuntimeContext.h
    RuntimeContext.cpp
    NodeHeartbeat.h
    NodeHeartbeat.cpp
)

target_link_libraries(cs_config PUBLIC
    chatserver_includes
    cs_grpc
    cs_log
)
