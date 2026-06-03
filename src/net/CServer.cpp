#include "CServer.h"
#include "AsioIOServicePool.h"
#include "CSession.h"
#include "Log.h"

// 构造函数：绑定端口、启动监听、开始接受连接
CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context), _port(port),
      _acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port))
{
    LOGI(LogModule::Net, "TCP server listening on port {}", _port);
    StartAccept();
}

CServer::~CServer()
{
    LOGI(LogModule::Net, "TCP server on port {} destroyed", _port);
}

boost::asio::io_context &CServer::ioContext()
{
    return _io_context;
}

// 获取当前所有会话的快照（线程安全）
void CServer::snapshotSessions(std::vector<std::shared_ptr<CSession>> &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    out.clear();
    out.reserve(_sessions.size());
    for (const auto &kv : _sessions)
    {
        out.push_back(kv.second);
    }
}

// 处理接受连接的结果
void CServer::HandleAccept(std::shared_ptr<CSession> new_session,
                           const boost::system::error_code &error)
{
    if (!error)
    {
        new_session->start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions[new_session->getSessionId()] = new_session;
        LOGI(LogModule::Net, "client connected, session={}, total={}",
             new_session->getSessionId(), _sessions.size());
    }
    else
    {
        LOGW(LogModule::Net, "accept failed: {}", error.message());
    }
    StartAccept();
}

// 启动异步接受连接
void CServer::StartAccept()
{
    auto &io_context = AsioIOServicePool::getInstance().getIOService();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->getSocket(),
                           [this, new_session](const boost::system::error_code &error) {
                               HandleAccept(new_session, error);
                           });
}

// 移除指定 uuid 的会话
void CServer::ClearSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(uuid);
    LOGI(LogModule::Net, "session removed, uuid={}, remaining={}", uuid, _sessions.size());
}
