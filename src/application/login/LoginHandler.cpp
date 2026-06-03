#include "LoginHandler.h"
#include "CSession.h"
#include "RuntimeContext.h"
#include "ConfigMgr.h"
#include "MySqlMgr.h"
#include "OfflineSyncService.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "UserCacheService.h"
#include "UserMgr.h"
#include "const.h"
#include "data.h"
#include "utils.h"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <vector>

void LoginHandler::handleLogin(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();

    Json::Value return_value;
    bool login_rsp_sent = false;
    utils::Defer defer([&return_value, session, &login_rsp_sent]() {
        if (!login_rsp_sent)
        {
            std::string return_str = return_value.toStyledString();
            session->send(return_str, MSG_CHAT_LOGIN_RSP);
        }
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

    auto user_info = std::make_shared<UserInfo>();
    bool b_base = UserCacheService::getByUid(uid, user_info);
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
    bool b_friend_list = getFriendList(uid, friend_list);
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
    const auto &self = RuntimeContext::getInstance().getNodeInfo();
    const std::string &server_name = self.name;
    StatusGrpcClient::getInstance().bindUserToNode(uid, server_name);
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

    login_rsp_sent = true;
    session->send(return_value.toStyledString(), MSG_CHAT_LOGIN_RSP);
    OfflineSyncService::syncAfterLogin(session, uid);
}

bool LoginHandler::getFriendApplyInfo(int touid, std::vector<std::shared_ptr<ApplyInfo>> &list)
{
    return MySqlMgr::getInstance().friends().getApplyList(touid, list);
}

bool LoginHandler::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    return MySqlMgr::getInstance().friends().getFriendList(uid, list);
}
