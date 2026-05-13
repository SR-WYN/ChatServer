#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "LogicSystem.h"
#include <csignal>
#include <mutex>
#include <thread>
#include "ChatServiceImpl.h"
#include "RedisMgr.h"
#include "const.h"
#include <grpcpp/grpcpp.h>


bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
    auto &cfg = ConfigMgr::getInstance();
    auto server_name = cfg["SelfServer"]["Name"];
    try
    {
        auto &pool = AsioIOServicePool::getInstance();
        RedisMgr::getInstance().hSet(RedisPrefix::LOGIN_COUNT, server_name, "0");
        std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        //监听端口和添加服务
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        //构建并启动gRPC服务器
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;

        std::thread grpc_server_thread([&server](){
            server->Wait();
        });
        
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, &pool, &server](auto, auto) {
            io_context.stop();
            pool.stop();
            server->Shutdown();
        });
        auto port_str = cfg["SelfServer"]["Port"];
        CServer s(io_context, atoi(port_str.c_str()));
        io_context.run();

        RedisMgr::getInstance().hDel(RedisPrefix::LOGIN_COUNT, server_name);
        RedisMgr::getInstance().close();
        grpc_server_thread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

    