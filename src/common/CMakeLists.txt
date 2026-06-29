add_library(common STATIC
    utils/utils.cpp
)

target_link_libraries(common PUBLIC
    chatserver_includes
    JsonCpp::JsonCpp
    spdlog::spdlog
)