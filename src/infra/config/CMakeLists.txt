add_library(cs_config STATIC
    ConfigMgr.cpp
    RuntimeContext.cpp
    NodeHeartbeat.cpp
)
add_dependencies(cs_config chat_proto_gen)
target_link_libraries(cs_config PUBLIC chatserver_includes Boost::filesystem Boost::system JsonCpp::JsonCpp spdlog::spdlog)
