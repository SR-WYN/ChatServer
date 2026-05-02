#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "utils.h"
#include "const.h"
#include <json/json.h>
#include "CSession.h"

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
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_ADDFRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext *context, const AuthFriendReq *request,
                                         AuthFriendRsp *reply)
{
    // todo
}

Status ChatServiceImpl::NotifyTextChatMsg(ServerContext *context, const TextChatMsgReq *request,
                                          TextChatMsgRsp *reply)
{
    // todo
}

bool ChatServiceImpl::getBaseInfo(std::string base_key, int uid,
                                  std::shared_ptr<UserInfo> &user_info)
{
    // todo
}