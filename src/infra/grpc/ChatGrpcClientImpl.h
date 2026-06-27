// ChatGrpcClientImpl.h - 跨节点 ChatServer RPC 客户端实现
#pragma once

#include "ChatGrpcClient.h"
#include "ChatConPool.h"
#include <memory>
#include <unordered_map>

class ChatGrpcClientImpl final : public ChatGrpcClient
{
public:
    ChatGrpcClientImpl();
    ~ChatGrpcClientImpl() override;

    AddFriendRsp NotifyAddFriend(const std::string& rpc_host, const std::string& rpc_port,
                                 const AddFriendReq& req) override;
    AuthFriendRsp NotifyAuthFriend(const std::string& rpc_host, const std::string& rpc_port,
                                   const AuthFriendReq& req) override;
    TextChatMsgRsp NotifyTextChatMsg(const std::string& rpc_host, const std::string& rpc_port,
                                     const TextChatMsgReq& req,
                                     const Json::Value& root_value) override;
    ImageChatMsgRsp NotifyImageChatMsg(const std::string& rpc_host, const std::string& rpc_port,
                                       const ImageChatMsgReq& req,
                                       const Json::Value& root_value) override;

private:
    ChatConPool& getOrCreatePool(const std::string& host, const std::string& port);

    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};
