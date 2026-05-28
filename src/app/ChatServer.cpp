#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ChatNodeSlotSelector.h"
#include "ChatRuntimeConfig.h"
#include "ChatServiceImpl.h"
#include "ConfigMgr.h"
#include "HeartBeatHandler.h"
#include "Log.h"
#include "LogicSystem.h"
#include "NodeHeartbeat.h"
#include "PersistWorker.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "const.h"
#include <csignal>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    try
    {
        ConfigMgr::getInstance();
        if (!Log::init("ChatServer", ConfigMgr::getInstance().getLogConfig()))
        {
            return 1;
        }
        Log::info(LogModule::App, "ChatServer starting");

        auto slot = ChatNodeSlotSelector::acquireAndRegister();
        if (!slot)
        {
            Log::error(LogModule::App, "failed to acquire chat node slot");
            Log::shutdown();
            return 1;
        }
        ChatRuntimeConfig::getInstance().setSelf(*slot);

        const auto &self = ChatRuntimeConfig::getInstance().self();
        auto &pool = AsioIOServicePool::getInstance();
        RedisMgr::getInstance().hSet(RedisPrefix::LOGIN_COUNT, self.name, "0");
        PersistWorker::getInstance().start();
        NodeHeartbeat::start();

        const std::string server_address = self.rpc_bind_host + ":" + self.rpc_port;
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        if (!server)
        {
            NodeHeartbeat::stop();
            StatusGrpcClient::getInstance().unregisterChatNode(self);
            return 1;
        }

        std::thread grpc_server_thread([&server]() { server->Wait(); });

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, &pool, &server](auto, auto) {
            io_context.stop();
            pool.stop();
            server->Shutdown();
        });

        CServer tcp_server(io_context, std::stoi(self.tcp_port));
        HeartBeatHandler::start(tcp_server);
        io_context.run();
        HeartBeatHandler::stop();

        NodeHeartbeat::stop();
        PersistWorker::getInstance().stop();
        StatusGrpcClient::getInstance().unregisterChatNode(self);
        RedisMgr::getInstance().hDel(RedisPrefix::LOGIN_COUNT, self.name);
        RedisMgr::getInstance().close();
        grpc_server_thread.join();
        Log::info(LogModule::App, "ChatServer stopped");
        Log::shutdown();
    }
    catch (std::exception &e)
    {
        Log::error(LogModule::App, "ChatServer exception: {}", e.what());
        if (ChatRuntimeConfig::getInstance().hasSelf())
        {
            NodeHeartbeat::stop();
            StatusGrpcClient::getInstance().unregisterChatNode(
                ChatRuntimeConfig::getInstance().self());
        }
        Log::shutdown();
        return 1;
    }
    return 0;
}
