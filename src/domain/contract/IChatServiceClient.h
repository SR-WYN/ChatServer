#pragma once

#include "data.h"
#include "message.pb.h"
#include <json/json.h>
#include <string>

using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

// 跨节点 ChatServer 客户端接口：节点间通知转发
class IChatServiceClient
{
public:
    virtual ~IChatServiceClient() = default;

    virtual AddFriendRsp NotifyAddFriend(const std::string &rpc_host,
                                         const std::string &rpc_port,
                                         const AddFriendReq &req) = 0;
    virtual AuthFriendRsp NotifyAuthFriend(const std::string &rpc_host,
                                           const std::string &rpc_port,
                                           const AuthFriendReq &req) = 0;
    virtual TextChatMsgRsp NotifyTextChatMsg(const std::string &rpc_host,
                                             const std::string &rpc_port,
                                             const TextChatMsgReq &req,
                                             const Json::Value &root_value) = 0;
};
