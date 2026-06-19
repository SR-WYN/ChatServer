add_library(cs_app STATIC
    interface/Login.h
    interface/UserSearch.h
    interface/Friend.h
    interface/ChatMessage.h
    interface/Heartbeat.h
    interface/LogicSystem.h
    interface/MySqlMgr.h
    interface/StatusGrpcClient.h
    interface/ChatGrpcClient.h
    interface/UserInfoCache.h
    interface/UserNodeRouteCache.h
    interface/UserSessionManager.h
    impl/LoginImpl.h
    impl/LoginImpl.cpp
    impl/UserSearchImpl.h
    impl/UserSearchImpl.cpp
    impl/FriendImpl.h
    impl/FriendImpl.cpp
    impl/ChatMessageImpl.h
    impl/ChatMessageImpl.cpp
    impl/HeartbeatImpl.h
    impl/HeartbeatImpl.cpp
    impl/LogicSystemImpl.h
    impl/LogicSystemImpl.cpp
)

target_link_libraries(cs_app PUBLIC
    chatserver_includes
    cs_base
    cs_log
    JsonCpp::JsonCpp
    Boost::system
)

add_dependencies(cs_app chat_proto_gen)
