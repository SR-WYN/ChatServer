add_library(cs_redis STATIC RedisMgr.cpp RedisConPool.cpp)
target_link_libraries(cs_redis PUBLIC chatserver_includes cs_config hiredis)
