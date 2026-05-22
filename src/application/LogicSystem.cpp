#include "LogicSystem.h"
#include "CSession.h"
#include "FriendHandler.h"
#include "HeartBeatHandler.h"
#include "LoginHandler.h"
#include "MsgNode.h"
#include "ChatHistoryHandler.h"
#include "TextChatHandler.h"
#include "UserHandler.h"
#include "const.h"
#include <iostream>
#include <memory>
#include <string>

LogicSystem::LogicSystem() : _b_stop(false)
{
    registerCallBacks();
    _worker_thread = std::thread([this]() {
        this->dealMsg();
    });
}

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    _consume.notify_all();
    _worker_thread.join();
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> msg)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _msg_que.push(msg);
    if (_msg_que.size() == 1)
    {
        lock.unlock();
        _consume.notify_one();
    }
}

void LogicSystem::dealMsg()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_msg_que.empty() && !_b_stop)
        {
            _consume.wait(lock);
        }

        if (_b_stop)
        {
            while (!_msg_que.empty())
            {
                auto msg_node = _msg_que.front();
                std::cout << "recv msg node is " << msg_node->getRecvNode()->getMsgId()
                          << std::endl;
                auto call_back_iter = _fun_callbacks.find(msg_node->getRecvNode()->getMsgId());
                if (call_back_iter == _fun_callbacks.end())
                {
                    _msg_que.pop();
                    continue;
                }
                HeartBeatHandler::touchSessionActivity(msg_node->getSession());
                call_back_iter->second(msg_node->getSession(), msg_node->getRecvNode()->getMsgId(),
                                       std::string(msg_node->getRecvNode()->getData(),
                                                   msg_node->getRecvNode()->getCurLen()));
                _msg_que.pop();
            }
            break;
        }

        auto msg_node = _msg_que.front();
        std::cout << "recv msg node is " << msg_node->getRecvNode()->getMsgId() << std::endl;
        auto call_back_iter = _fun_callbacks.find(msg_node->getRecvNode()->getMsgId());
        if (call_back_iter == _fun_callbacks.end())
        {
            _msg_que.pop();
            std::cout << "not found call back for msg id " << msg_node->getRecvNode()->getMsgId()
                      << std::endl;
            continue;
        }

        HeartBeatHandler::touchSessionActivity(msg_node->getSession());
        call_back_iter->second(
            msg_node->getSession(), msg_node->getRecvNode()->getMsgId(),
            std::string(msg_node->getRecvNode()->getData(), msg_node->getRecvNode()->getCurLen()));
        _msg_que.pop();
    }
}

void LogicSystem::registerCallBacks()
{
    _fun_callbacks[MSG_CHAT_LOGIN] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            LoginHandler::handleLogin(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_SEARCH_USER_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            UserHandler::handleSearchUser(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_ADD_FRIEND_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            FriendHandler::handleAddFriend(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_AUTH_FRIEND_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            FriendHandler::handleAuthFriend(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_TEXT_CHAT_MSG_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            TextChatHandler::handleTextChat(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_CHAT_HISTORY_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            ChatHistoryHandler::handleHistory(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_HEARTBEAT_PING] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            HeartBeatHandler::handlePing(session, msg_id, msg_data);
        };
}
