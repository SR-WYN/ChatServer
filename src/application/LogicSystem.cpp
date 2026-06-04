#include "LogicSystem.h"
#include "CSession.h"
#include "ChatMessageHandler.h"
#include "ChatQueryHandler.h"
#include "FriendHandler.h"
#include "HeartBeatHandler.h"
#include "LoginHandler.h"
#include "MsgNode.h"
#include "ThreadPoolMgr.h"
#include "UserHandler.h"
#include "const.h"
#include <iostream>
#include <memory>
#include <string>

LogicSystem::LogicSystem()
{
    registerCallBacks();
}

LogicSystem::~LogicSystem()
{
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> msg)
{
    ThreadPoolMgr::getInstance().enqueueLogic([this, msg]() {
        auto call_back_iter = _fun_callbacks.find(msg->getRecvNode()->getMsgId());
        if (call_back_iter == _fun_callbacks.end())
        {
            return;
        }

        HeartBeatHandler::touchSessionActivity(msg->getSession());
        call_back_iter->second(
            msg->getSession(), msg->getRecvNode()->getMsgId(),
            std::string(msg->getRecvNode()->getData(), msg->getRecvNode()->getCurLen()));
    });
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
            ChatMessageHandler::handleTextChat(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_CHAT_HISTORY_REQ] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            ChatQueryHandler::handleHistory(session, msg_id, msg_data);
        };
    _fun_callbacks[MSG_HEARTBEAT_PING] =
        [](std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
            HeartBeatHandler::handlePing(session, msg_id, msg_data);
        };
}
