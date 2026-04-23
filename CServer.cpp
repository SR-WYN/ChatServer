#include "CServer.h"
#include "AsioIOServicePool.h"
#include "CSession.h"
#include <future>
#include <iostream>

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context), _port(port),
      _acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port))
{
    std::cout << "Server start success, listen on port: " << _port << std::endl;
    StartAccept();
}

CServer::~CServer()
{
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

void CServer::ClearSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(uuid);
}