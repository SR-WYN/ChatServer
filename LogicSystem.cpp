#include "LogicSystem.h"
#include "CSession.h"
#include "ConfigMgr.h"
#include "MsgNode.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "UserMgr.h"
#include "const.h"
#include "data.h"
#include "utils.h"
#include <memory>

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
    _fun_callbacks[MSG_SEARCH_USER_REQ] = [this](std::shared_ptr<CSession> session,
                                                 const short &msg_id, const std::string &msg_data) {
        this->searchUserHandler(session, msg_id, msg_data);
    };
}

void LogicSystem::loginHandler(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    std::cout << "user login uid is  " << uid << " user token  is " << token << std::endl;

    Json::Value return_value;
    utils::Defer defer([&return_value, session]() {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_CHAT_LOGIN_RSP);
    });

    // 从redis中获取用户token是否正确
    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::getInstance().get(token_key, token_value);
    if (!success)
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    if (token_value != token)
    {
        return_value["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }

    return_value["error"] = ErrorCodes::SUCCESS;

    std::string base_key = RedisPrefix::USER_BASE_INFO + uid_str;
    auto user_info = std::make_shared<UserInfo>();
    bool b_base = getBaseInfo(base_key, uid, user_info);
    if (!b_base)
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    return_value["uid"] = uid;
    return_value["pwd"] = user_info->pwd;
    return_value["name"] = user_info->name;
    return_value["email"] = user_info->email;
    return_value["nick"] = user_info->nick;
    return_value["desc"] = user_info->desc;
    return_value["sex"] = user_info->sex;
    return_value["icon"] = user_info->icon;

    // 从数据库获取申请列表

    // 获取好友列表

    auto server_name = ConfigMgr::getInstance()["SelfServer"]["Name"];
    // 将登录数量增加
    auto rd_res = RedisMgr::getInstance().hGet(RedisPrefix::LOGIN_COUNT, server_name);
    int count = 0;
    if (!rd_res.empty())
    {
        count = std::stoi(rd_res);
    }
    count++;

    auto count_str = std::to_string(count);
    RedisMgr::getInstance().hSet(RedisPrefix::LOGIN_COUNT, server_name.c_str(), count_str.c_str(),
                                 count_str.size());
    // session绑定用户uid
    session->setUserId(uid);

    // 为用户设置登录ip server 的名字
    std::string ipkey = RedisPrefix::USERIPPREFIX + uid_str;
    RedisMgr::getInstance().set(ipkey, server_name);

    // uid和session绑定管理，方便以后踢人操作
    UserMgr::getInstance().setUserSession(uid, session);
}

void LogicSystem::searchUserHandler(std::shared_ptr<CSession> session, const short &msg_id,
                                    const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value result;
    utils::Defer defer([&result, session]() {
        std::string return_str = result.toStyledString();
        session->send(return_str, MSG_SEARCH_USER_RSP);
    });

    if (!reader.parse(msg_data, root) || !root.isMember("uid"))
    {
        result["error"] = ErrorCodes::ERROR_JSON;
        return;
    }
    auto uid_str = root["uid"].asString();
    if (root["uid"].isInt())
    {
        getUserByUid(uid_str, result);
    }
    else
    {
        getUserByName(uid_str, result);
    }
}

bool LogicSystem::getBaseInfo(const std::string &base_key, int uid,
                              std::shared_ptr<UserInfo> user_info)
{
    if (!user_info)
    {
        return false;
    }

    std::string raw;
    if (RedisMgr::getInstance().get(base_key, raw))
    {
        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(raw, root) || !root.isObject())
        {
            return false;
        }
        user_info->uid = uid;
        user_info->name = root.isMember("name") ? root["name"].asString() : "";
        user_info->pwd = root.isMember("pwd") ? root["pwd"].asString() : "";
        user_info->email = root.isMember("email") ? root["email"].asString() : "";
        user_info->nick = root.isMember("nick") ? root["nick"].asString() : user_info->name;
        user_info->desc = root.isMember("desc") ? root["desc"].asString() : "";
        user_info->sex = root.isMember("sex") ? root["sex"].asInt() : 0;
        user_info->back = root.isMember("back") ? root["back"].asString() : "";
        return true;
    }

    auto db_user = MySqlMgr::getInstance().getUserInfo(uid);
    if (!db_user)
    {
        return false;
    }
    *user_info = *db_user;
    if (user_info->nick.empty())
    {
        user_info->nick = user_info->name;
    }

    Json::Value cache_root;
    cache_root["uid"] = user_info->uid;
    cache_root["name"] = user_info->name;
    cache_root["pwd"] = user_info->pwd;
    cache_root["email"] = user_info->email;
    cache_root["nick"] = user_info->nick;
    cache_root["desc"] = user_info->desc;
    cache_root["sex"] = user_info->sex;
    cache_root["icon"] = user_info->icon;
    Json::FastWriter writer;
    const std::string cache_json = writer.write(cache_root);
    (void)RedisMgr::getInstance().set(base_key, cache_json);
    if (!user_info->name.empty())
    {
        (void)RedisMgr::getInstance().set(RedisPrefix::USER_NAME_INFO + user_info->name, cache_json);
    }

    return true;
}

void LogicSystem::getUserByUid(const std::string &uid_str, Json::Value &result)
{
    result["error"] = ErrorCodes::SUCCESS;
    std::string base_key = RedisPrefix::USER_BASE_INFO + uid_str;
    std::string info_str = "";
    bool b_base = RedisMgr::getInstance().get(base_key, info_str);
    if (b_base)
    {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();

        std::cout << "get user by uid " << uid << " name " << name << " pwd " << pwd << " email "
                  << email << " nick " << nick << " desc " << desc << " sex " << sex << " icon "
                  << icon << std::endl;

        result["uid"] = uid;
        result["name"] = name;
        result["pwd"] = pwd;
        result["email"] = email;
        result["nick"] = nick;
        result["desc"] = desc;
        result["sex"] = sex;
        result["icon"] = icon;
        return;
    }
    auto uid = std::stoi(uid_str);
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MySqlMgr::getInstance().getUserInfo(uid);
    if (user_info == nullptr)
    {
        result["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["name"] = user_info->name;
    redis_root["pwd"] = user_info->pwd;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    const std::string cache_str = redis_root.toStyledString();
    RedisMgr::getInstance().set(base_key, cache_str);
    if (!user_info->name.empty())
    {
        RedisMgr::getInstance().set(RedisPrefix::USER_NAME_INFO + user_info->name, cache_str);
    }
}

void LogicSystem::getUserByName(const std::string &name_str, Json::Value &result)
{
    result["error"] = ErrorCodes::SUCCESS;
    std::string name_key = RedisPrefix::USER_NAME_INFO + name_str;
    std::string info_str = "";
    bool b_name = RedisMgr::getInstance().get(name_key, info_str);
    if (b_name)
    {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();

        std::cout << "get user by name " << name_str << " uid " << uid << " name " << name
                  << " pwd " << pwd << " email " << email << " nick " << nick << " desc " << desc
                  << " sex " << sex << " icon " << icon << std::endl;

        result["uid"] = uid;
        result["name"] = name;
        result["pwd"] = pwd;
        result["email"] = email;
        result["nick"] = nick;
        result["desc"] = desc;
        result["sex"] = sex;
        result["icon"] = icon;
        return;
    }
    auto user_info = MySqlMgr::getInstance().getUserInfo(name_str);
    if (user_info == nullptr)
    {
        result["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    const std::string uid_str = std::to_string(user_info->uid);
    std::string base_key = RedisPrefix::USER_BASE_INFO + uid_str;
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["name"] = user_info->name;
    redis_root["pwd"] = user_info->pwd;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    const std::string cache_str = redis_root.toStyledString();
    RedisMgr::getInstance().set(base_key, cache_str);
    if (!user_info->name.empty())
    {
        RedisMgr::getInstance().set(RedisPrefix::USER_NAME_INFO + user_info->name, cache_str);
    }
}