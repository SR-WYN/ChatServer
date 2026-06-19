add_library(cs_log STATIC
    Log.h
    Log.cpp
    LogModule.h
)

target_link_libraries(cs_log PUBLIC
    chatserver_includes
    spdlog::spdlog
)
