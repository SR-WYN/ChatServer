add_library(cs_net STATIC AsioIOServicePool.cpp CServer.cpp CSession.cpp MsgNode.cpp)
target_link_libraries(cs_net PUBLIC chatserver_includes Boost::system Boost::thread JsonCpp::JsonCpp)
