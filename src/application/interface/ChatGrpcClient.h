// ChatGrpcClient.h - 跨节点 ChatServer RPC 客户端接口
#pragma once

#include "data.h"
#include "message.pb.h"
#include <json/json.h>
#include <string>

using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ImageChatMsgReq;
using message::ImageChatMsgRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

class ChatGrpcClient
{
public:
    virtual ~ChatGrpcClient() = default;

    virtual AddFriendRsp NotifyAddFriend(const std::string& rpc_host, const std::string& rpc_port,
                                         const AddFriendReq& req) = 0;
    virtual AuthFriendRsp NotifyAuthFriend(const std::string& rpc_host,
                                           const std::string& rpc_port,
                                           const AuthFriendReq& req) = 0;
    virtual TextChatMsgRsp NotifyTextChatMsg(const std::string& rpc_host,
                                             const std::string& rpc_port,
                                             const TextChatMsgReq& req,
                                             const Json::Value& root_value) = 0;
    virtual ImageChatMsgRsp NotifyImageChatMsg(const std::string& rpc_host,
                                               const std::string& rpc_port,
                                               const ImageChatMsgReq& req,
                                               const Json::Value& root_value) = 0;
};
