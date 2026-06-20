add_library(cs_net STATIC
    CServer.h
    CServer.cpp
    CSession.h
    CSession.cpp
    MsgNode.h
    MsgNode.cpp
)

target_link_libraries(cs_net PUBLIC
    chatserver_includes
    cs_config
    cs_log
    cs_cache
    cs_app
    Boost::system
    Boost::thread
    Boost::filesystem
)
