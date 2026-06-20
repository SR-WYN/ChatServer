// CServer.cpp
#include "CServer.h"
#include "CSession.h"
#include "Log.h"
#include "ThreadPoolMgr.h"

CServer::CServer(boost::asio::io_context& io_context, short port,
                 std::shared_ptr<LogicSystem> logic_system,
                 std::shared_ptr<StatusGrpcClient> status_client,
                 std::shared_ptr<UserNodeRouteCache> route_cache,
                 std::shared_ptr<UserSessionManager> session_manager)
    : _io_context(io_context),
      _port(port),
      _acceptor(io_context,
                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port)),
      _logic_system(std::move(logic_system)),
      _status_client(std::move(status_client)),
      _route_cache(std::move(route_cache)),
      _session_manager(std::move(session_manager))
{
    LOGI(LogModule::Net, "TCP server constructed on port {}", _port);
}

CServer::~CServer()
{
    LOGI(LogModule::Net, "TCP server on port {} destroyed", _port);
}

boost::asio::io_context& CServer::ioContext()
{
    return _io_context;
}

void CServer::snapshotSessions(std::vector<std::shared_ptr<CSession>>& out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    out.clear();
    out.reserve(_sessions.size());
    for (const auto& kv : _sessions)
    {
        out.push_back(kv.second);
    }
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session,
                           const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions[new_session->getSessionId()] = new_session;
        LOGI(LogModule::Net, "client connected, session={}, total={}", new_session->getSessionId(),
             _sessions.size());
    }
    else
    {
        LOGW(LogModule::Net, "accept failed: {}", error.message());
    }
    start();
}

void CServer::start()
{
    LOGI(LogModule::Net, "TCP server starts accepting on port {}", _port);
    auto& io_context = ThreadPoolMgr::getInstance().getIoService();
    std::shared_ptr<CSession> new_session =
        std::make_shared<CSession>(io_context, shared_from_this(), _logic_system, _status_client,
                                   _route_cache, _session_manager);
    _acceptor.async_accept(new_session->getSocket(),
                           [this, new_session](const boost::system::error_code& error) {
                               HandleAccept(new_session, error);
                           });
}

void CServer::ClearSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(uuid);
    LOGI(LogModule::Net, "session removed, uuid={}, remaining={}", uuid, _sessions.size());
}
