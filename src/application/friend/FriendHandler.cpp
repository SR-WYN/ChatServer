#include "FriendHandler.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include "UserCacheService.h"
#include "UserMgr.h"
#include "const.h"
#include "data.h"
#include "message.pb.h"
#include "utils.h"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>

void FriendHandler::handleAddFriend(std::shared_ptr<CSession> session, const short &msg_id,
                                    const std::string &msg_data)
{
    (void)msg_id;
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
    utils::Defer defer([&return_value, session]() {
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

    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = UserCacheService::getByUid(uid, apply_info);

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

void FriendHandler::handleAuthFriend(std::shared_ptr<CSession> session, const short &msg_id,
                                     const std::string &msg_data)
{
    (void)msg_id;
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
    bool b_info = UserCacheService::getByUid(applicant_uid, user_info);
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
    utils::Defer defer([&return_value, session] {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_AUTH_FRIEND_RSP);
    });

    std::string alias_applicant_for_accepter;
    MySqlMgr::getInstance().getFriendApplyAlias(applicant_uid, accepter_uid,
                                                alias_applicant_for_accepter);

    MySqlMgr::getInstance().authFriendApply(applicant_uid, accepter_uid);
    MySqlMgr::getInstance().addFriend(applicant_uid, accepter_uid, alias_applicant_for_accepter,
                                      alias_name);

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
            auto peer_info = std::make_shared<UserInfo>();
            bool b_into = UserCacheService::getByUid(accepter_uid, peer_info);
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
