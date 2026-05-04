#include "LogicSystem.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "MsgNode.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "UserMgr.h"
#include "const.h"
#include "data.h"
#include "message.pb.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
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
    _fun_callbacks[MSG_ADD_FRIEND_REQ] = [this](std::shared_ptr<CSession> session,
                                                const short &msg_id, const std::string &msg_data) {
        this->addFriendHandler(session, msg_id, msg_data);
    };
    _fun_callbacks[MSG_AUTH_FRIEND_REQ] = [this](std::shared_ptr<CSession> session,
                                                 const short &msg_id, const std::string &msg_data) {
        this->authFriendHandler(session, msg_id, msg_data);
    };
    _fun_callbacks[MSG_TEXT_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session,
                                                   const short &msg_id, const std::string &msg_data) {
        this->chatTextMsgHandler(session, msg_id, msg_data);
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
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    bool b_apply = getFriendApplyInfo(uid, apply_list);
    if (b_apply)
    {
        for (auto &apply : apply_list)
        {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["status"] = apply->_status;
            obj["alias_name"] = apply->alias_name;
            return_value["apply_list"].append(obj);
        }
    }
    // 获取好友列表
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = getFriendList(uid,friend_list);
    for (auto &friend_element : friend_list)
    {
        Json::Value obj;
        obj["name"] = friend_element->name;
        obj["uid"] = friend_element->uid;
        obj["icon"] = friend_element->icon;
        obj["nick"] = friend_element->nick;
        obj["sex"] = friend_element->sex;
        obj["desc"] = friend_element->desc;
        obj["alias_name"] = friend_element->alias_name;
        return_value["friend_list"].append(obj);
    }
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

    // 暂时没有redisTTL机制

    // std::string raw;
    // if (RedisMgr::getInstance().get(base_key, raw))
    // {
    //     Json::Reader reader;
    //     Json::Value root;
    //     if (!reader.parse(raw, root) || !root.isObject())
    //     {
    //         return false;
    //     }
    //     user_info->uid = uid;
    //     user_info->name = root.isMember("name") ? root["name"].asString() : "";
    //     user_info->pwd = root.isMember("pwd") ? root["pwd"].asString() : "";
    //     user_info->email = root.isMember("email") ? root["email"].asString() : "";
    //     user_info->nick = root.isMember("nick") ? root["nick"].asString() : user_info->name;
    //     user_info->desc = root.isMember("desc") ? root["desc"].asString() : "";
    //     user_info->sex = root.isMember("sex") ? root["sex"].asInt() : 0;
    //     user_info->back = root.isMember("back") ? root["back"].asString() : "";
    //     return true;
    // }

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
        (void)RedisMgr::getInstance().set(RedisPrefix::USER_NAME_INFO + user_info->name,
                                          cache_json);
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

void LogicSystem::addFriendHandler(std::shared_ptr<CSession> session, const short &msg_id,
                                   const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto name = root["apply_name"].asString();
    auto alias_name = root["alias_name"].asString();
    auto touid = root["touid"].asInt();
    std::cout << "add friend uid is " << uid << " name is " << name << " alias_name is "
              << alias_name << " touid is " << touid << std::endl;

    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    utils::Defer defer([this, &return_value, session]() {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_ADD_FRIEND_RSP);
    });

    MySqlMgr::getInstance().addFriendApply(uid, touid, alias_name);

    // 查询Redis 查找touid对应的server ip
    auto to_str = std::to_string(touid);
    auto to_ip_key = RedisPrefix::USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::getInstance().get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        std::cout << "get to ip failed" << std::endl;
        return;
    }

    std::string base_key = RedisPrefix::USER_BASE_INFO + std::to_string(uid);
    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = getBaseInfo(base_key, uid, apply_info);

    auto &cfg = ConfigMgr::getInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    // 直接通知对方有申请消息（字段与 ChatServiceImpl::NotifyAddFriend 一致）
    if (to_ip_value == self_name)
    {
        auto to_user_session = UserMgr::getInstance().getSession(touid);
        if (to_user_session)
        {
            Json::Value notify;
            notify["error"] = ErrorCodes::SUCCESS;
            notify["applyuid"] = uid;
            notify["name"] = name;
            notify["desc"] = "";
            if (b_info)
            {
                notify["icon"] = apply_info->icon;
                notify["nick"] = apply_info->nick;
                notify["sex"] = apply_info->sex;
            }
            else
            {
                notify["icon"] = "";
                notify["nick"] = "";
                notify["sex"] = 0;
            }
            notify["alias_name"] = alias_name;
            std::string return_str = notify.toStyledString();
            to_user_session->send(return_str, MSG_NOTIFY_ADDFRIEND_REQ);
        }
        return;
    }

    message::AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_name(name);
    add_req.set_desc("");
    add_req.set_touid(touid);
    if (b_info)
    {
        add_req.set_icon(apply_info->icon.c_str());
        add_req.set_nick(apply_info->nick.c_str());
        add_req.set_sex(apply_info->sex);
    }
    add_req.set_alias_name(alias_name);
    // 发送请求到对方服务器
    ChatGrpcClient::getInstance().NotifyAddFriend(to_ip_value, add_req);
}

bool LogicSystem::getFriendApplyInfo(int touid, std::vector<std::shared_ptr<ApplyInfo>> &list)
{
    // 从数据库获取申请列表
    return MySqlMgr::getInstance().getApplyList(touid, list);
}

void LogicSystem::authFriendHandler(std::shared_ptr<CSession> session, const short &msg_id,
                                    const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int applicant_uid = root["fromuid"].asInt();
    const int accepter_uid = root["touid"].asInt();
    auto alias_name = root["alias_name"].asString();
    std::cout << "auth friend: applicant " << applicant_uid << " accepter " << accepter_uid
              << std::endl;
    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    auto user_info = std::make_shared<UserInfo>();

    // 给同意方返回：刚通过的好友是申请人 applicant 的资料
    std::string base_key = RedisPrefix::USER_BASE_INFO + std::to_string(applicant_uid);
    bool b_info = getBaseInfo(base_key, applicant_uid, user_info);
    if (b_info)
    {
        return_value["name"] = user_info->name;
        return_value["nick"] = user_info->nick;
        return_value["icon"] = user_info->icon;
        return_value["sex"] = user_info->sex;
        return_value["uid"] = applicant_uid;
        return_value["alias_name"] = alias_name;
    }
    else
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
    }
    utils::Defer defer([this, &return_value, session] {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_AUTH_FRIEND_RSP);
    });

    std::string alias_applicant_for_accepter;
    MySqlMgr::getInstance().getFriendApplyAlias(applicant_uid, accepter_uid, alias_applicant_for_accepter);

    MySqlMgr::getInstance().authFriendApply(applicant_uid, accepter_uid);
    MySqlMgr::getInstance().addFriend(applicant_uid, accepter_uid, alias_applicant_for_accepter, alias_name);

    std::string applicant_peer_alias;
    MySqlMgr::getInstance().getFriendAlias(applicant_uid, accepter_uid, applicant_peer_alias);
    // 通知申请人所在 Chat 节点（与 ChatServiceImpl::NotifyAuthFriend 中 touid 为收包用户一致）
    auto to_ip_key = RedisPrefix::USERIPPREFIX + std::to_string(applicant_uid);
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::getInstance().get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        std::cout << "get to ip failed" << std::endl;
        return;
    }
    auto &cfg = ConfigMgr::getInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name)
    {
        auto peer_session = UserMgr::getInstance().getSession(applicant_uid);
        if (peer_session)
        {
            Json::Value notify;
            notify["error"] = ErrorCodes::SUCCESS;
            notify["fromuid"] = accepter_uid;
            notify["touid"] = applicant_uid;
            std::string peer_key = RedisPrefix::USER_BASE_INFO + std::to_string(accepter_uid);
            auto peer_info = std::make_shared<UserInfo>();
            bool b_into = getBaseInfo(peer_key, accepter_uid, peer_info);
            if (b_into)
            {
                notify["name"] = peer_info->name;
                notify["nick"] = peer_info->nick;
                notify["icon"] = peer_info->icon;
                notify["sex"] = peer_info->sex;
                notify["alias_name"] = applicant_peer_alias;
            }
            else
            {
                notify["error"] = ErrorCodes::UID_INVALID;
            }
            std::string return_str = notify.toStyledString();
            peer_session->send(return_str, MSG_NOTIFY_AUTH_FRIEND_REQ);
        }
        return;
    }
    AuthFriendReq auth_req;
    auth_req.set_fromuid(accepter_uid);
    auth_req.set_touid(applicant_uid);

    ChatGrpcClient::getInstance().NotifyAuthFriend(to_ip_value, auth_req);
}

bool LogicSystem::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    return MySqlMgr::getInstance().getFriendList(uid, list);
}

void LogicSystem::chatTextMsgHandler(std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    if (root.isNull() || root.empty())
    {
        std::cout << "chat text msg data is null or empty" << std::endl;
        return;
    }
    auto fromuid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    const Json::Value text_array = root["text_array"];
    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    return_value["fromuid"] = touid;
    return_value["touid"] = fromuid;
    return_value["text_array"] = text_array;

    utils::Defer defer([this,&return_value,session](){
        std::string return_str = return_value.toStyledString();
        session->send(return_str,MSG_TEXT_CHAT_MSG_RSP);
    });

    // 查询redis 查找touid对应的server ip
    auto to_str = std::to_string(touid);
    auto to_ip_key = RedisPrefix::USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::getInstance().get(to_ip_key,to_ip_value);
    if (!b_ip)
    {
        return;
    }
    auto &cfg = ConfigMgr::getInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name)
    {
        auto session = UserMgr::getInstance().getSession(touid);
        if (session)
        {
            std::string return_str = return_value.toStyledString();
            session->send(return_str,MSG_TEXT_CHAT_MSG_RSP);
        }
        return;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(fromuid);
    text_msg_req.set_touid(touid);
    for (const auto &text_obj : text_array)
    {
        auto content = text_obj["content"].asString();
        auto msgid = text_obj["msgid"].asString();
        std::cout << "content is " << content << " msgid is " << msgid << std::endl;
        auto *text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);
    }
    ChatGrpcClient::getInstance().NotifyTextChatMsg(to_ip_value, text_msg_req, return_value);
}