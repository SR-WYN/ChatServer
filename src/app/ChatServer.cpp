#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ChatServiceImpl.h"
#include "ConfigMgr.h"
#include "HeartBeatHandler.h"
#include "Log.h"
#include "LogicSystem.h"
#include "NodeHeartbeat.h"
#include "PersistWorker.h"
#include "RedisMgr.h"
#include "RuntimeContext.h"
#include "ServiceLocator.h"
#include "StatusGrpcClient.h"
#include "UserInfoCache.h"
#include "UserInfoCacheImpl.h"
#include "const.h"
#include "utils.h"
#include <csignal>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    try
    {
        // ---- 1. 初始化基础设施 ----
        ConfigMgr::getInstance();
        // if (!Log::init("ChatServer", ConfigMgr::getInstance().getLogConfig()))
        // {
        //     return 1;
        // }
        LOGI(LogModule::App, "ChatServer starting");

        // ---- 1.5 注册业务服务 ----
        ServiceLocator::registerService<UserInfoCache>(std::make_shared<UserInfoCacheImpl>());

        // ---- 2. 向 StatusServer 注册当前节点 ----
        // 遍历配置槽位，找到端口可用且注册成功的 slot
        auto slot = RuntimeContext::tryRegisterNode();
        if (!slot)
        {
            LOGE(LogModule::App, "failed to acquire chat node slot");
            Log::shutdown();
            return 1;
        }
        RuntimeContext::getInstance().setNodeInfo(*slot);

        // ---- 2.5 初始化全局雪花 ID 生成器 ----
        // 从 slot_key 解析节点 ID（slot0 → 0, slot1 → 1, ...）
        {
            uint64_t node_id = 0;
            const std::string &slot_key = slot->slot_key;
            // 去掉 "slot" 前缀，取数字部分
            if (slot_key.size() > 4)
            {
                node_id = static_cast<uint64_t>(std::stoul(slot_key.substr(4)));
            }
            g_snowflake = new utils::SnowflakeId(node_id);
            LOGI(LogModule::App, "SnowflakeId initialized | node_id={} slot_key={}", node_id,
                 slot_key);
        }

        // ---- 3. 初始化工作线程 ----
        const auto &self = RuntimeContext::getInstance().getNodeInfo();
        auto &pool = AsioIOServicePool::getInstance();
        PersistWorker::getInstance().start();
        NodeHeartbeat::start();

        // ---- 4. 注册信号处理（优雅退出） ----
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, &pool](auto, auto) {
            io_context.stop();
            pool.stop();
        });

        // ---- 5. 启动 gRPC 服务（供其他后端服务调用） ----
        const std::string server_address = self.rpc_host + ":" + self.rpc_port;
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
        std::thread grpc_server_thread([&server]() {
            server->Wait();
        });

        // ---- 6. 启动 TCP 服务（供客户端连接） ----
        CServer tcp_server(io_context, std::stoi(self.tcp_port));
        HeartBeatHandler::start(tcp_server);
        io_context.run();
        HeartBeatHandler::stop();

        // ---- 7. 清理资源 ----
        NodeHeartbeat::stop();
        PersistWorker::getInstance().stop();
        StatusGrpcClient::getInstance().unregisterChatNode(self);
        RedisMgr::getInstance().close();
        grpc_server_thread.join();
        delete g_snowflake;
        g_snowflake = nullptr;
        LOGI(LogModule::App, "ChatServer stopped");
        Log::shutdown();
    }
    catch (std::exception &e)
    {
        LOGE(LogModule::App, "ChatServer exception: {}", e.what());
        if (RuntimeContext::getInstance().isInitialized())
        {
            NodeHeartbeat::stop();
            StatusGrpcClient::getInstance().unregisterChatNode(
                RuntimeContext::getInstance().getNodeInfo());
        }
        delete g_snowflake;
        g_snowflake = nullptr;
        Log::shutdown();
        return 1;
    }
    return 0;
}
