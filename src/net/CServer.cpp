#include "CServer.h"
#include "AsioIOServicePool.h"
#include "CSession.h"
#include "const.h"
#include <chrono>
#include <future>
#include <iostream>
#include <vector>

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context), _port(port),
      _acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port)),
      _idle_sweep_timer(io_context)
{
    std::cout << "Server start success, listen on port: " << _port << std::endl;
    StartAccept();
    scheduleIdleSweep();
}

CServer::~CServer()
{
    boost::system::error_code ec;
    _idle_sweep_timer.cancel(ec);
    std::cout << "Server destruct" << std::endl;
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session,
                           const boost::system::error_code &error)
{
    if (!error)
    {
        new_session->start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions[new_session->getSessionId()] = new_session;
        std::cout << "New session accepted: " << new_session->getSessionId() << std::endl;
    }
    else
    {
        std::cout << "Accept error: " << error.message() << std::endl;
    }
    StartAccept();
}

void CServer::StartAccept()
{
    auto &io_context = AsioIOServicePool::getInstance().getIOService();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(_io_context, this);
    _acceptor.async_accept(new_session->getSocket(),
                           [this, new_session](const boost::system::error_code &error) {
                               HandleAccept(new_session, error);
                           });
}

void CServer::scheduleIdleSweep()
{
    _idle_sweep_timer.expires_after(std::chrono::seconds(10));
    _idle_sweep_timer.async_wait([this](const boost::system::error_code &ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            std::cout << "idle sweep timer: " << ec.message() << std::endl;
        }
        else
        {
            runIdleSweep();
        }
        scheduleIdleSweep();
    });
}

void CServer::runIdleSweep()
{
    std::vector<std::shared_ptr<CSession>> snapshot;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        snapshot.reserve(_sessions.size());
        for (const auto &kv : _sessions)
        {
            snapshot.push_back(kv.second);
        }
    }
    const auto limit = std::chrono::seconds(SESSION_APP_IDLE_SECONDS);
    for (const auto &s : snapshot)
    {
        if (!s)
        {
            continue;
        }
        if (s->appIdleAge() >= limit)
        {
            std::cout << "closing idle session " << s->getSessionId() << std::endl;
            s->close();
        }
    }
}

void CServer::ClearSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(uuid);
}
