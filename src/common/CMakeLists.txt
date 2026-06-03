add_library(cs_common STATIC utils.cpp Log.cpp)
target_link_libraries(cs_common PUBLIC chatserver_includes Boost::system Boost::filesystem spdlog::spdlog)
