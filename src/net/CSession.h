#pragma once
#include "const.h"
#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <queue>

using tcp = boost::asio::ip::tcp;

class SendNode;
class RecvNode;
class MsgNode;
class CServer;

// TCP 会话：管理单个客户端连接，负责消息的收发和生命周期管理
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

    // body_len 是包体长度，内部会自动补齐协议包头
    void send(const char *msg, short body_len, short msgid);
    void send(std::string msg, short msgid);
    void close();
    void touchActivity();
    std::chrono::milliseconds appIdleAge() const;

private:
    // 异步读取消息头（HEAD_TOTAL_LEN 字节），解析 msg_id 和 body_len
    void asyncReadHead();

    // 异步读取消息体（body_len 字节），投递到逻辑队列处理
    void asyncReadBody(int body_len);

    // 处理写完成回调，继续发送队列中下一条消息
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> shared_self);

    tcp::socket _socket;
    std::string _session_id;
    char _data[MAX_LENGTH];
    CServer *_server;
    bool _b_close;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    std::shared_ptr<RecvNode> _recv_msg_node;  // 当前正在接收的消息
    bool _b_head_parse;
    std::shared_ptr<MsgNode> _recv_head_node;  // 当前正在接收的消息头
    int _user_uid;
    std::atomic<std::uint64_t> _last_activity_ms{0};
};

class LogicNode
{
public:
    LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
    std::shared_ptr<RecvNode> getRecvNode();
    std::shared_ptr<CSession> getSession();
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recv_node;
};
