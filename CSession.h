#include "const.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <queue>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

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
    void send(char *msg, short max_length, short msgid);
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