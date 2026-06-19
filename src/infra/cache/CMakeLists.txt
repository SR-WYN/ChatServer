add_library(cs_cache STATIC
    LRUCache.h
    UserInfoCacheImpl.h
    UserInfoCacheImpl.cpp
    UserNodeRouteCacheImpl.h
    UserNodeRouteCacheImpl.cpp
    UserSessionManagerImpl.h
    UserSessionManagerImpl.cpp
)

target_link_libraries(cs_cache PUBLIC
    chatserver_includes
    cs_mysql
)
