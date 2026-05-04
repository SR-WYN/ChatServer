#include "ChatServiceImpl.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <json/json.h>
#include <json/reader.h>
#include <memory>
#include "MySqlMgr.h"


ChatServiceImpl::ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext *context, const AddFriendReq *request,
                                        AddFriendRsp *reply)
{ // 查找用户是否在本服务器
    auto touid = request->touid();
    auto session = UserMgr::getInstance().getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    // 用户不在内存中直接返回
    if (session == nullptr)
    {
        return Status::OK;
    }
    // 用户在内存中则发送通知
    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["applyuid"] = request->applyuid();
    notify["name"] = request->name();
    notify["desc"] = request->desc();
    notify["icon"] = request->icon();
    notify["sex"] = request->sex();
    notify["nick"] = request->nick();
    notify["alias_name"] = request->alias_name();
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_ADDFRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext *context, const AuthFriendReq *request,
                                         AuthFriendRsp *reply)
{
    // 查找用户是否在本服务器
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::getInstance().getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    // 用户不在内存中直接返回
    if (session == nullptr)
    {
        return Status::OK;
    }

    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    return_value["fromuid"] = fromuid;
    return_value["touid"] = touid;

    std::string base_key = RedisPrefix::USER_BASE_INFO + std::to_string(fromuid);
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = getBaseInfo(base_key, fromuid, user_info);
    if (b_info)
    {
        return_value["name"] = user_info->name;
        return_value["nick"] = user_info->nick;
        return_value["icon"] = user_info->icon;
        return_value["sex"] = user_info->sex;
        std::string peer_alias;
        MySqlMgr::getInstance().getFriendAlias(touid, fromuid, peer_alias);
        return_value["alias_name"] = peer_alias;
    }
    else
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
    }
    std::string return_str = return_value.toStyledString();
    session->send(return_str, MSG_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(ServerContext *context, const TextChatMsgReq *request,
                                          TextChatMsgRsp *reply)
{
    // todo
    return Status::OK;
}

bool ChatServiceImpl::getBaseInfo(std::string base_key, int uid,
                                  std::shared_ptr<UserInfo> &user_info)
{
    // 优先redis中查
    std::string info_str = "";
    bool b_base = RedisMgr::getInstance().get(base_key, info_str);
    if (b_base)
    {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        user_info->uid = root["uid"].asInt();
        user_info->name = root["name"].asString();
        user_info->pwd = root["pwd"].asString();
        user_info->email = root["email"].asString();
        user_info->nick = root["nick"].asString();
        user_info->desc = root["desc"].asString();
        user_info->sex = root["sex"].asInt();
        user_info->icon = root["icon"].asString();
        user_info->back.clear();
        user_info->alias_name.clear();
        std::cout << "user login uid is " << user_info->uid << " name is " << user_info->name
                  << " pwd is " << user_info->pwd << " email is " << user_info->email << " nick is "
                  << user_info->nick << " desc is " << user_info->desc << " sex is "
                  << user_info->sex << " icon is " << user_info->icon << std::endl;
        return true;
    }
    // redis中没有则从数据库中查
    std::shared_ptr<UserInfo> user_info_data = nullptr;
    user_info_data = MySqlMgr::getInstance().getUserInfo(uid);
    if (!user_info_data)
    {
        return false;
    }

    user_info = user_info_data;

    // 将用户信息写入redis
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["name"] = user_info->name;
    redis_root["pwd"] = user_info->pwd;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;
    std::string redis_str = redis_root.toStyledString();
    RedisMgr::getInstance().set(base_key, redis_str);
    return true;
}