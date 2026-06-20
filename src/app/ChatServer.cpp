// ChatServer.cpp - 组合根
#include "CServer.h"
#include "ChatConPool.h"
#include "ChatGrpcClient.h"
#include "ChatGrpcClientImpl.h"
#include "ChatGrpcServiceImpl.h"
#include "ChatMessage.h"
#include "ChatMessageImpl.h"
#include "ConfigMgr.h"
#include "Friend.h"
#include "FriendImpl.h"
#include "Heartbeat.h"
#include "HeartbeatImpl.h"
#include "IdGenerator.h"
#include "Log.h"
#include "LogicSystem.h"
#include "LogicSystemImpl.h"
#include "Login.h"
#include "LoginImpl.h"
#include "MySqlMgr.h"
#include "MySqlMgrImpl.h"
#include "MySqlPool.h"
#include "RuntimeContext.h"
#include "SnowflakeIdGenerator.h"
#include "StatusConPool.h"
#include "StatusGrpcClient.h"
#include "StatusGrpcClientImpl.h"
#include "ThreadPoolMgr.h"
#include "UserInfoCache.h"
#include "UserInfoCacheImpl.h"
#include "UserNodeRouteCache.h"
#include "UserNodeRouteCacheImpl.h"
#include "UserSearch.h"
#include "UserSearchImpl.h"
#include "UserSessionManager.h"
#include "UserSessionManagerImpl.h"
#include "const.h"
#include "utils.h"
#include <csignal>
#include <grpcpp/grpcpp.h>

int main()
{
    std::unique_ptr<grpc::Server> server;

    try
    {
        if (!Log::init("ChatServer", ConfigMgr::getInstance().getLogConfig()))
        {
            return 1;
        }
        ConfigMgr::getInstance();
        LOGI(LogModule::App, "ChatServer starting");

        // ---- 1. 基础设施 ----
        MySqlPool::initOnce();
        auto mysql_pool = MySqlPool::getInstancePtr();
        auto mysql_mgr = std::shared_ptr<MySqlMgr>(std::make_shared<MySqlMgrImpl>(mysql_pool));

        auto status_client =
            std::shared_ptr<StatusGrpcClient>(std::make_shared<StatusGrpcClientImpl>());
        auto chat_client = std::shared_ptr<ChatGrpcClient>(std::make_shared<ChatGrpcClientImpl>());

        auto session_manager =
            std::shared_ptr<UserSessionManager>(std::make_shared<UserSessionManagerImpl>());
        auto user_info_cache =
            std::shared_ptr<UserInfoCache>(std::make_shared<UserInfoCacheImpl>(mysql_mgr));
        auto route_cache =
            std::shared_ptr<UserNodeRouteCache>(std::make_shared<UserNodeRouteCacheImpl>());

        const auto &runtime_context = RuntimeContext::getInstance();

        // ---- 2. 注册当前节点 ----
        auto slot = RuntimeContext::tryRegisterNode(status_client.get());
        if (!slot)
        {
            LOGE(LogModule::App, "failed to acquire chat node slot");
            Log::shutdown();
            return 1;
        }
        RuntimeContext::getInstance().setNodeInfo(*slot);

        uint64_t node_id = 0;
        const std::string &slot_key = slot->slot_key;
        if (slot_key.size() > 4)
        {
            node_id = static_cast<uint64_t>(std::stoul(slot_key.substr(4)));
        }
        auto id_generator =
            std::shared_ptr<IdGenerator>(std::make_shared<SnowflakeIdGenerator>(node_id));
        LOGI(LogModule::App, "SnowflakeId initialized | node_id={} slot_key={}", node_id, slot_key);

        // ---- 3. 启动工作线程 ----
        const auto &self = runtime_context.getNodeInfo();

        // 初始化线程池管理器（所有线程池在此创建，包括 IO 池）
        ThreadPoolMgr::getInstance();

        // 节点心跳 → 线程池管理
        ThreadPoolMgr::getInstance().runNodeHeartbeat([client = status_client]() {
            while (client && RuntimeContext::getInstance().isInitialized())
            {
                const auto &nodeInfo = RuntimeContext::getInstance().getNodeInfo();
                client->heartbeatChatNode(nodeInfo.name, nodeInfo.instance_uid);
                for (int i = 0; i < 100; ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        });

        // ---- 4. 应用层 ----
        auto login = std::shared_ptr<Login>(std::make_shared<LoginImpl>(
            status_client, user_info_cache, mysql_mgr, session_manager, runtime_context));
        auto user_search =
            std::shared_ptr<UserSearch>(std::make_shared<UserSearchImpl>(user_info_cache));
        auto friend_business = std::shared_ptr<Friend>(
            std::make_shared<FriendImpl>(status_client, chat_client, session_manager, route_cache,
                                         mysql_mgr, user_info_cache, runtime_context));
        auto chat_message = std::shared_ptr<ChatMessage>(std::make_shared<ChatMessageImpl>(
            session_manager, route_cache, mysql_mgr, status_client, chat_client, id_generator,
            runtime_context));
        auto heartbeat = std::shared_ptr<Heartbeat>(std::make_shared<HeartbeatImpl>());

        // ---- 5. 消息路由器 ----
        auto logic_system = std::shared_ptr<LogicSystem>(
            std::make_shared<LogicSystemImpl>(ThreadPoolMgr::getInstance()));
        logic_system->registerHandler(MSG_CHAT_LOGIN, [login](std::shared_ptr<CSession> session,
                                                              const std::string &msg_data) {
            login->handle(session, msg_data);
        });
        logic_system->registerHandler(
            MSG_SEARCH_USER_REQ,
            [user_search](std::shared_ptr<CSession> session, const std::string &msg_data) {
                user_search->handle(session, msg_data);
            });
        logic_system->registerHandler(
            MSG_ADD_FRIEND_REQ,
            [friend_business](std::shared_ptr<CSession> session, const std::string &msg_data) {
                friend_business->handleAddFriend(session, msg_data);
            });
        logic_system->registerHandler(
            MSG_AUTH_FRIEND_REQ,
            [friend_business](std::shared_ptr<CSession> session, const std::string &msg_data) {
                friend_business->handleAuthFriend(session, msg_data);
            });
        logic_system->registerHandler(
            MSG_TEXT_CHAT_MSG_REQ,
            [chat_message](std::shared_ptr<CSession> session, const std::string &msg_data) {
                chat_message->handleTextChat(session, msg_data);
            });
        logic_system->registerHandler(
            MSG_CHAT_HISTORY_REQ,
            [chat_message](std::shared_ptr<CSession> session, const std::string &msg_data) {
                chat_message->handleHistory(session, msg_data);
            });
        logic_system->registerHandler(
            MSG_HEARTBEAT_PING,
            [heartbeat](std::shared_ptr<CSession> session, const std::string &msg_data) {
                heartbeat->handlePing(session, msg_data);
            });

        // ---- 6. 注册信号处理 ----
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](auto, auto) {
            io_context.stop();
        });

        // ---- 7. 启动 gRPC 服务 ----
        const std::string server_address = self.rpc_host + ":" + self.rpc_port;
        ChatGrpcServiceImpl grpc_service(session_manager, user_info_cache, mysql_mgr);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&grpc_service);
        server = builder.BuildAndStart();
        if (!server)
        {
            ThreadPoolMgr::getInstance().joinNodeHeartbeat();
            status_client->unregisterChatNode(self);
            return 1;
        }

        // ---- 8. 启动 TCP 服务 ----
        auto tcp_server =
            std::make_shared<CServer>(io_context, std::stoi(self.tcp_port), logic_system,
                                      status_client, route_cache, session_manager);
        tcp_server->start();
        heartbeat->start(*tcp_server);

        // MySQL 健康检查 → 挂在 acceptor 的 io_context 上，用定时器替代独立线程
        MySqlPool::getInstance().startHealthCheck(io_context);

        // 主线程运行 acceptor io_context（阻塞直到收到信号）
        io_context.run();

        // ---- 9. 清理资源 ----
        heartbeat->stop();
        ThreadPoolMgr::getInstance().joinNodeHeartbeat();
        status_client->unregisterChatNode(self);
        if (server)
        {
            server->Shutdown();
        }
        LOGI(LogModule::App, "ChatServer stopped");
        Log::shutdown();
    }
    catch (std::exception &e)
    {
        LOGE(LogModule::App, "ChatServer exception: {}", e.what());
        if (server)
        {
            server->Shutdown();
        }
        if (RuntimeContext::getInstance().isInitialized())
        {
            ThreadPoolMgr::getInstance().joinNodeHeartbeat();
            auto fallback_status = std::make_shared<StatusGrpcClientImpl>();
            fallback_status->unregisterChatNode(RuntimeContext::getInstance().getNodeInfo());
        }
        Log::shutdown();
        return 1;
    }
    return 0;
}
