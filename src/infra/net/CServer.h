// CServer.h - TCP 服务器
#pragma once
#include "LogicSystem.h"
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CSession;
class StatusGrpcClient;
class UserNodeRouteCache;
class UserSessionManager;

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& io_context, short port,
            std::shared_ptr<LogicSystem> logic_system,
            std::shared_ptr<StatusGrpcClient> status_client,
            std::shared_ptr<UserNodeRouteCache> route_cache,
            std::shared_ptr<UserSessionManager> session_manager);
    ~CServer();

    void ClearSession(std::string uuid);
    boost::asio::io_context& ioContext();
    void snapshotSessions(std::vector<std::shared_ptr<CSession>>& out);

private:
    void HandleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& error);
    void StartAccept();

    boost::asio::io_context& _io_context;
    short _port;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;

    std::shared_ptr<LogicSystem> _logic_system;
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<UserNodeRouteCache> _route_cache;
    std::shared_ptr<UserSessionManager> _session_manager;
};
