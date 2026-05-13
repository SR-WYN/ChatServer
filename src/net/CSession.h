#pragma once
#include "const.h"
#include <boost/asio.hpp>
#include <memory>
#include <queue>

using tcp = boost::asio::ip::tcp;

class SendNode;
class RecvNode;
class MsgNode;
class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context &io_context, CServer *server);
    ~CSession();
    tcp::socket &getSocket();
    std::string &getSessionId();
    void setUserId(int user_uid);
    int getUserId();
    void start();
    // body_len 是包体长度，SendNode 内部会自动补齐协议包头。
    void send(const char *msg, short body_len, short msgid);
    void send(std::string msg, short msgid);
    void close();
    std::shared_ptr<CSession> shared_self();
    void AsyncReadBody(int length);
    void AsyncReadHead(int total_len);

private:
    void asyncReadFull(std::size_t max_length,
                       std::function<void(const boost::system::error_code &, std::size_t)> handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len,
                      std::function<void(const boost::system::error_code &, std::size_t)> handler);
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> shared_self);
    tcp::socket _socket;
    std::string _session_id;
    char _data[MAX_LENGTH];
    CServer *_server;
    bool _b_close;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    // 收到的消息结构
    std::shared_ptr<RecvNode> _recv_msg_node;
    bool _b_head_parse;
    // 收到的头部结构
    std::shared_ptr<MsgNode> _recv_head_node;
    int _user_uid;
};

class LogicNode
{
public:
    LogicNode(std::shared_ptr<CSession>,std::shared_ptr<RecvNode>);
    std::shared_ptr<RecvNode> getRecvNode();
    std::shared_ptr<CSession> getSession();
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recv_node;
};