add_library(cs_application STATIC
    LogicSystem.cpp
    mysql/MySqlPool.cpp
    mysql/MySqlMgr.cpp
    redis/RedisMgr.cpp
    login/LoginHandler.cpp
    user/UserHandler.cpp
    user/UserDao.cpp
    user/UserInfoCacheImpl.cpp
    user/UserMgr.cpp
    user/HeartBeatHandler.cpp
    friend/FriendHandler.cpp
    friend/FriendDao.cpp
    chat/ChatMessageHandler.cpp
    chat/ChatMessageDao.cpp
    chat/ChatQueryHandler.cpp
    chat/ChatServiceImpl.cpp
    chat/PersistWorker.cpp
)
add_dependencies(cs_application chat_proto_gen)
target_link_libraries(cs_application PUBLIC
    chatserver_includes
    cs_domain
    cs_net
    cs_common
    cs_config
    cs_grpc
    mysqlcppconn
    redis++
)
