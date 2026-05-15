#pragma once
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <map>
#include <mutex>

class CSession;

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context &io_context, short port);
    ~CServer();
    void ClearSession(std::string);

private:
    void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code &error);
    void StartAccept();
    void scheduleIdleSweep();
    void runIdleSweep();

    boost::asio::io_context &_io_context;
    short _port;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::steady_timer _idle_sweep_timer;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
};
