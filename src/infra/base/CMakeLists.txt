add_library(cs_base STATIC
    Singleton.h
    ThreadPool.h
    ThreadPool.cpp
    ThreadPoolMgr.h
    ThreadPoolMgr.cpp
    SnowflakeIdGenerator.h
    SnowflakeIdGenerator.cpp
)

target_link_libraries(cs_base PUBLIC
    chatserver_includes
    common
    spdlog::spdlog
)
