// CSession.h - TCP 会话
#pragma once
#include "const.h"
#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <queue>

using tcp = boost::asio::ip::tcp;

class CServer;
class LogicSystem;
class SendNode;
class RecvNode;
class MsgNode;
class StatusGrpcClient;
class UserNodeRouteCache;
class UserSessionManager;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, std::shared_ptr<CServer> server,
             std::shared_ptr<LogicSystem> logic_system,
             std::shared_ptr<StatusGrpcClient> status_client,
             std::shared_ptr<UserNodeRouteCache> route_cache,
             std::shared_ptr<UserSessionManager> session_manager);
    ~CSession();

    tcp::socket& getSocket();
    std::string& getSessionId();
    void setUserId(int user_uid);
    int getUserId();
    void start();
    void send(const char* msg, short body_len, short msgid);
    void send(std::string msg, short msgid);
    void close();
    void closeAfterSend();
    void setKicked(bool kicked);
    bool isKicked() const;
    void touchActivity();
    std::chrono::milliseconds appIdleAge() const;

private:
    void asyncReadHead();
    void asyncReadBody(int body_len);
    void handleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    tcp::socket _socket;
    std::string _session_id;
    char _data[MAX_LENGTH];
    std::shared_ptr<CServer> _server;
    bool _b_close;
    std::atomic<bool> _kicked{false};
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    std::shared_ptr<RecvNode> _recv_msg_node;
    bool _b_head_parse;
    std::shared_ptr<MsgNode> _recv_head_node;
    int _user_uid;
    std::atomic<std::uint64_t> _last_activity_ms{0};

    std::shared_ptr<LogicSystem> _logic_system;
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<UserNodeRouteCache> _route_cache;
    std::shared_ptr<UserSessionManager> _session_manager;
};

class LogicNode
{
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node);
    std::shared_ptr<RecvNode> getRecvNode();
    std::shared_ptr<CSession> getSession();

private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recv_node;
};
