#include "LogicSystem.h"
#include "CSession.h"
#include "MsgNode.h"
#include "StatusGrpcClient.h"
#include "data.h"
#include "utils.h"
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include "MySqlMgr.h"

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

        call_back_iter->second(
            msg_node->getSession(), msg_node->getRecvNode()->getMsgId(),
            std::string(msg_node->getRecvNode()->getData(), msg_node->getRecvNode()->getCurLen()));
        _msg_que.pop();
    }
}

void LogicSystem::registerCallBacks()
{
    _fun_callbacks[MSG_CHAT_LOGIN] = [this](std::shared_ptr<CSession> session, const short &msg_id,
                                            const std::string &msg_data) {
        this->loginHandler(session, msg_id, msg_data);
    };
}

void LogicSystem::loginHandler(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    std::cout << "user login uid is  " << root["uid"].asInt() << " user token  is "
              << root["token"].asString() << std::endl;
    // 从状态服务器获取token匹配是否准确
    auto rsp = StatusGrpcClient::getInstance().login(root["uid"].asInt(), root["token"].asString());
    Json::Value result;
    utils::Defer defer([this,&result,session]{
        std::string return_str = result.toStyledString();
        session->send(return_str,MSG_CHAT_LOGIN_RSP);
    });

    result["error"] = rsp.error();
    if (rsp.error() != ErrorCodes::SUCCESS)
    {
        return;
    }

    //内存中查询用户信息
    auto find_iter = _users.find(root["uid"].asInt());
    std::shared_ptr<UserInfo> user_info = nullptr;
    if (find_iter == _users.end())
    {
        //查不到就查询数据库
        user_info = MySqlMgr::getInstance().getUserInfo(root["uid"].asInt());
        if (user_info == nullptr)
        {
            result["error"] = ErrorCodes::UID_INVALID;
            return;
        }
        _users[uid] = user_info;
    }
    else 
    {
        user_info = find_iter->second;
    }

    session->setUserId(uid);
    result["uid"] = uid;
    result["token"] = rsp.token();
    result["name"] = user_info->name;
}