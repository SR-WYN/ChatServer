#include "ChatServiceImpl.h"
#include "CSession.h"
#include "UserCacheService.h"
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

    auto user_info = std::make_shared<UserInfo>();
    bool b_info = UserCacheService::getByUid(fromuid, user_info);
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
    auto touid = request->touid();
    auto session = UserMgr::getInstance().getSession(touid);
    reply->set_error(ErrorCodes::SUCCESS);

    // 用户不在内存中直接返回
    if (session == nullptr)
    {
        return Status::OK;
    }

    // 用户在内存中则发送通知
    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = request->fromuid();
    notify["touid"] = request->touid();

    Json::Value text_array;
    for (const auto& text_data : request->textmsgs())
    {
        Json::Value element;
        element["msgid"] = text_data.msgid();
        element["content"] = text_data.msgcontent();
        text_array.append(element);
    }
    notify["text_array"] = text_array;
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_TEXT_CHAT_MSG_REQ);
    return Status::OK;
}