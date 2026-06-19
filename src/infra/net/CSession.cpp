// CSession.cpp
#include "CSession.h"
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "LogicSystem.h"
#include "MsgNode.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include "UserNodeRouteCache.h"
#include "UserSessionManager.h"
#include "const.h"
#include "utils.h"
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstring>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <limits>
#include <memory>
#include <mutex>

namespace
{
std::uint64_t steady_ms()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}
} // namespace

CSession::CSession(boost::asio::io_context& io_context, std::shared_ptr<CServer> server,
                   std::shared_ptr<LogicSystem> logic_system,
                   std::shared_ptr<StatusGrpcClient> status_client,
                   std::shared_ptr<UserNodeRouteCache> route_cache,
                   std::shared_ptr<UserSessionManager> session_manager)
    : _socket(io_context),
      _server(std::move(server)),
      _b_close(false),
      _b_head_parse(false),
      _user_uid(0),
      _logic_system(std::move(logic_system)),
      _status_client(std::move(status_client)),
      _route_cache(std::move(route_cache)),
      _session_manager(std::move(session_manager))
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(uuid);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() = default;

tcp::socket& CSession::getSocket()
{
    return _socket;
}

std::string& CSession::getSessionId()
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

void CSession::touchActivity()
{
    _last_activity_ms.store(steady_ms(), std::memory_order_relaxed);
}

std::chrono::milliseconds CSession::appIdleAge() const
{
    const std::uint64_t now = steady_ms();
    const std::uint64_t last = _last_activity_ms.load(std::memory_order_relaxed);
    if (last == 0 || now < last)
    {
        return std::chrono::milliseconds(0);
    }
    return std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(now - last));
}

void CSession::start()
{
    touchActivity();
    asyncReadHead();
}

void CSession::asyncReadHead()
{
    auto self = shared_from_this();
    ::memset(_data, 0, MAX_LENGTH);
    boost::asio::async_read(
        _socket, boost::asio::buffer(_data, HEAD_TOTAL_LEN),
        [self, this](const boost::system::error_code& ec, std::size_t) {
            try
            {
                if (ec)
                {
                    LOGW(LogModule::Net, "read head failed, session={}, err={}", _session_id,
                         ec.message());
                    close();
                    _server->ClearSession(_session_id);
                    return;
                }

                _recv_head_node->clear();
                memcpy(_recv_head_node->getData(), _data, HEAD_TOTAL_LEN);

                short msg_id = 0;
                memcpy(&msg_id, _recv_head_node->getData(), HEAD_ID_LEN);
                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);

                short msg_len = 0;
                memcpy(&msg_len, _recv_head_node->getData() + HEAD_ID_LEN, HEAD_DATA_LEN);
                msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

                if (msg_len > MAX_LENGTH)
                {
                    LOGW(LogModule::Net, "msg too long, session={}, len={}", _session_id, msg_len);
                    _server->ClearSession(_session_id);
                    return;
                }

                _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
                asyncReadBody(msg_len);
            }
            catch (std::exception& e)
            {
                LOGE(LogModule::Net, "asyncReadHead exception, session={}, err={}", _session_id,
                     e.what());
            }
        });
}

void CSession::asyncReadBody(int body_len)
{
    auto self = shared_from_this();
    ::memset(_data, 0, MAX_LENGTH);
    boost::asio::async_read(
        _socket, boost::asio::buffer(_data, body_len),
        [self, this, body_len](const boost::system::error_code& ec, std::size_t) {
            try
            {
                if (ec)
                {
                    LOGW(LogModule::Net, "read body failed, session={}, err={}", _session_id,
                         ec.message());
                    close();
                    _server->ClearSession(_session_id);
                    return;
                }

                memcpy(_recv_msg_node->getData(), _data, body_len);
                _recv_msg_node->setCurLen(body_len + _recv_msg_node->getCurLen());
                ::memset(_recv_msg_node->getData() + _recv_msg_node->getTotalLen(), 0, 1);

                _logic_system->postMsgToQue(
                    std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));

                asyncReadHead();
            }
            catch (std::exception& e)
            {
                LOGE(LogModule::Net, "asyncReadBody exception, session={}, err={}", _session_id,
                     e.what());
            }
        });
}

void CSession::send(const char* msg, short body_len, short msg_id)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size >= MAX_SENDQUE)
    {
        LOGW(LogModule::Net, "send queue full, session={}", _session_id);
        return;
    }

    if (body_len <= 0 || body_len > MAX_LENGTH)
    {
        return;
    }

    _send_que.push(std::make_shared<SendNode>(msg, body_len, msg_id));
    if (_send_que.size() > 1)
    {
        return;
    }
    auto& msg_node = _send_que.front();
    auto self = shared_from_this();
    boost::asio::async_write(_socket,
                             boost::asio::buffer(msg_node->getData(), msg_node->getTotalLen()),
                             [self, this](const boost::system::error_code& ec, std::size_t) {
                                 handleWrite(ec, self);
                             });
}

void CSession::send(std::string msg, short msgid)
{
    if (msg.size() > static_cast<std::size_t>(std::numeric_limits<short>::max()))
    {
        return;
    }

    send(msg.data(), static_cast<short>(msg.size()), msgid);
}

void CSession::handleWrite(const boost::system::error_code& ec,
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
                auto& msg_node = _send_que.front();
                auto self = shared_from_this();
                boost::asio::async_write(
                    _socket, boost::asio::buffer(msg_node->getData(), msg_node->getTotalLen()),
                    [self, this](const boost::system::error_code& ec, std::size_t) {
                        handleWrite(ec, self);
                    });
            }
        }
        else
        {
            LOGW(LogModule::Net, "write failed, session={}, err={}", _session_id, ec.message());
        }
    }
    catch (std::exception& e)
    {
        LOGE(LogModule::Net, "handleWrite exception, session={}, err={}", _session_id, e.what());
        close();
        _server->ClearSession(_session_id);
    }
}

void CSession::close()
{
    if (_b_close)
    {
        return;
    }

    if (_user_uid > 0)
    {
        _status_client->unbindUser(_user_uid);

        if (_route_cache)
        {
            _route_cache->invalidate(_user_uid);
        }

        _session_manager->removeUserSession(_user_uid);
        LOGI(LogModule::Net, "session closed, uid={}, session={}", _user_uid, _session_id);
        _user_uid = 0;
    }

    _socket.close();
    _b_close = true;
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node)
    : _session(std::move(session)), _recv_node(std::move(recv_node))
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
