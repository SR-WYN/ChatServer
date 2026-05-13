add_library(cs_config STATIC ConfigMgr.cpp)
target_link_libraries(cs_config PUBLIC chatserver_includes Boost::filesystem JsonCpp::JsonCpp)
