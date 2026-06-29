add_library(common STATIC
    utils/utils.cpp
    utils/snowflake_id.cpp
)

target_link_libraries(common PUBLIC
    chatserver_includes
    Boost::system
    spdlog::spdlog
)