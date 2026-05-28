add_library(cs_config STATIC
    ConfigMgr.cpp
    ChatRuntimeConfig.cpp
    ChatNodeSlotSelector.cpp
    NodeHeartbeat.cpp
)
target_link_libraries(cs_config PUBLIC chatserver_includes Boost::filesystem Boost::system JsonCpp::JsonCpp spdlog::spdlog)
