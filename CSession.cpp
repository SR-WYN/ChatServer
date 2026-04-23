#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "MsgNode.h"
#include "const.h"
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>

CSession::CSession(boost::asio::io_context &io_context, CServer *server)
    : _socket(io_context), _server(server), _b_close(false), _b_head_parse(false)
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(uuid);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession()
{
    std::cout << "CSession destruct" << std::endl;
}

tcp::socket &CSession::getSocket()
{
    return _socket;
}

std::string &CSession::getSessionId()
{
    return _session_id;
}

void CSession::setUserId(int user_uid)
{
    _user_uid = user_uid;
}

int CSession::getUserId()
{
    return _user_uid;
}

void CSession::start()
{
    AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::AsyncReadHead(int total_len)
{
    auto self = shared_from_this();
    asyncReadFull(HEAD_TOTAL_LEN,
                  [self, this](const boost::system::error_code &ec, std::size_t bytes_transfered) {
                      try
                      {
                          if (ec)
                          {
                              std::cout << "handle read failed,error is " << ec.what() << std::endl;
                              close();
                              _server->ClearSession(_session_id);
                              return;
                          }

                          if (bytes_transfered < HEAD_TOTAL_LEN)
                          {
                              std::cout << "read length not match, read [" << bytes_transfered
                                        << "] , total [" << HEAD_TOTAL_LEN << "]" << std::endl;
                              close();
                              _server->ClearSession(_session_id);
                              return;
                          }

                          _recv_head_node->clear();
                          memcpy(_recv_head_node->getData(), _data, bytes_transfered);

                          // 获取头部MSGID数据
                          short msg_id = 0;
                          memcpy(&msg_id, _recv_head_node->getData(), HEAD_ID_LEN);
                          // 网络字节序转化为本地字节序
                          msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                          std::cout << "msg_id is " << msg_id << std::endl;

                          if (msg_id > MAX_LENGTH)
                          {
                              std::cout << "invalid msg_id, msg_id is " << msg_id << std::endl;
                              _server->ClearSession(_session_id);
                              return;
                          }

                          short msg_len = 0;
                          memcpy(&msg_len, _recv_head_node->getData() + HEAD_ID_LEN, HEAD_DATA_LEN);
                          msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
                          std::cout << "msg_len is " << msg_len << std::endl;

                          if (msg_len > MAX_LENGTH)
                          {
                              std::cout << "invalid msg_len, msg_len is " << msg_len << std::endl;
                              _server->ClearSession(_session_id);
                              return;
                          }

                          _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
                          AsyncReadBody(msg_len);
                      }
                      catch (std::exception &e)
                      {
                          std::cout << "exception is " << e.what() << std::endl;
                      }
                  });
}

void CSession::AsyncReadBody(int total_len)
{
    auto self = shared_from_this();
    asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code &ec,
                                                     std::size_t bytes_transfered) {
        try
        {
            if (ec)
            {
                std::cout << "handle read failed , error is " << ec.what() << std::endl;
                close();
                _server->ClearSession(_session_id);
                return;
            }

            if (bytes_transfered < total_len)
            {
                std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
                          << total_len << "]" << std::endl;
                close();
                _server->ClearSession(_session_id);
                return;
            }

            memcpy(_recv_msg_node->getData(), _data, bytes_transfered);
            _recv_msg_node->setCurLen(bytes_transfered + _recv_msg_node->getCurLen());
            ::memset(_recv_msg_node->getData() + _recv_msg_node->getTotalLen(), 0, 1);
            std::cout << "receive data is " << _recv_msg_node->getData() << std::endl;
            // 此处将消息投递到逻辑队列里
            LogicSystem::getInstance().postMsgToQue(
                std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
            AsyncReadHead(HEAD_TOTAL_LEN);
        }
        catch (std::exception &e)
        {
            std::cout << "exception is " << e.what() << std::endl;
        }
    });
}

void CSession::send(char *msg, short max_length, short msg_id)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size >= MAX_SENDQUE)
    {
        std::cout << "send queue is full, send queue size is " << send_que_size << std::endl;
        return;
    }

    _send_que.push(std::make_shared<SendNode>(msg, max_length, msg_id));
    if (_send_que.size() > 1)
    {
        return;
    }
    auto &msg_node = _send_que.front();
    auto self = shared_from_this();
    boost::asio::async_write(_socket,
                             boost::asio::buffer(msg_node->getData(), msg_node->getTotalLen()),
                             [self, this](const boost::system::error_code &ec, std::size_t) {
                                 handleWrite(ec, self);
                             });
}

void CSession::send(std::string msg, short msgid)
{
    if (msg.size() > static_cast<std::size_t>(std::numeric_limits<short>::max()))
    {
        std::cout << "send msg is too long, msg size is " << msg.size() << std::endl;
        return;
    }

    send(const_cast<char *>(msg.data()), static_cast<short>(msg.size()), msgid);
}

void CSession::handleWrite(const boost::system::error_code &ec,
                           std::shared_ptr<CSession> shared_self)
{
    try
    {
        if (!ec)
        {
            std::lock_guard<std::mutex> lock(_send_lock);
            _send_que.pop();
            if (!_send_que.empty())
            {
                auto &msg_node = _send_que.front();
                auto self = shared_from_this();
                boost::asio::async_write(
                    _socket, boost::asio::buffer(msg_node->getData(), msg_node->getTotalLen()),
                    [self, this](const boost::system::error_code &ec, std::size_t) {
                        handleWrite(ec, self);
                    });
            }
        }
    }
    catch (std::exception &e)
    {
        std::cout << "exception is " << e.what() << std::endl;
        close();
        _server->ClearSession(_session_id);
    }
}

void CSession::close()
{
    _socket.close();
    _b_close = true;
}

void CSession::asyncReadFull(
    std::size_t max_length,
    std::function<void(const boost::system::error_code &, std::size_t)> handler)
{
    ::memset(_data, 0, MAX_LENGTH);
    asyncReadLen(0, max_length, handler);
}

void CSession::asyncReadLen(
    std::size_t read_len, std::size_t total_len,
    std::function<void(const boost::system::error_code &, std::size_t)> handler)
{
    auto self = shared_from_this();
    _socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
                            [read_len, total_len, handler, self](
                                const boost::system::error_code &ec, std::size_t bytes_transfered) {
                                if (ec)
                                {
                                    handler(ec, read_len + bytes_transfered);
                                    return;
                                }

                                if (read_len + bytes_transfered >= total_len)
                                {
                                    handler(ec, read_len + bytes_transfered);
                                    return;
                                }

                                self->asyncReadLen(read_len + bytes_transfered, total_len, handler);
                            });
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node)
    : _session(session), _recv_node(recv_node)
{
}

std::shared_ptr<RecvNode> LogicNode::getRecvNode()
{
    return _recv_node;
}

std::shared_ptr<CSession> LogicNode::getSession()
{
    return _session;
}