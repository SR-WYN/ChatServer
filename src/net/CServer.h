#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CSession;

// TCP 服务器：监听端口、接受客户端连接、管理会话生命周期
class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context &io_context, short port);
    ~CServer();

    // 移除指定 uuid 的会话
    void ClearSession(std::string uuid);

    // 获取关联的 io_context
    boost::asio::io_context &ioContext();

    // 获取当前所有会话的快照（线程安全）
    void snapshotSessions(std::vector<std::shared_ptr<CSession>> &out);

private:
    // 处理接受连接的结果
    void HandleAccept(std::shared_ptr<CSession> session, const boost::system::error_code &error);

    // 启动异步接受连接
    void StartAccept();

    boost::asio::io_context &_io_context;                       // 关联的 io_context
    short _port;                                                // 监听端口
    boost::asio::ip::tcp::acceptor _acceptor;                   // TCP 接受器
    std::map<std::string, std::shared_ptr<CSession>> _sessions; // 当前活跃会话（uuid -> session）
    std::mutex _mutex;                                          // 保护 _sessions 的互斥锁
};
