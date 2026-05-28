#include "ChatGrpcClient.h"
#include "ChatConPool.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <grpcpp/client_context.h>
#include <mutex>
#include <sstream>

namespace
{
std::string endpointKey(const std::string &host, const std::string &port)
{
    return host + ":" + port;
}
} // namespace

ChatGrpcClient::ChatGrpcClient() = default;

ChatGrpcClient::~ChatGrpcClient() = default;

ChatConPool &ChatGrpcClient::getOrCreatePool(const std::string &host, const std::string &port)
{
    const std::string key = endpointKey(host, port);
    auto find_iter = _pools.find(key);
    if (find_iter != _pools.end())
    {
        return *find_iter->second;
    }
    auto pool = std::make_unique<ChatConPool>(5, host, port);
    auto &ref = *pool;
    _pools.emplace(key, std::move(pool));
    return ref;
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(const std::string &rpc_host, const std::string &rpc_port,
                                             const AddFriendReq &req)
{
    AddFriendRsp rsp;
    utils::Defer defer([&rsp, &req]() {
        rsp.set_error(ErrorCodes::SUCCESS);
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
        return rsp;
    }
    auto &pool = getOrCreatePool(rpc_host, rpc_port);
    ClientContext context;
    auto stub = pool.getConnection();
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() { pool.returnConnection(std::move(stub)); });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
    }
    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(const std::string &rpc_host, const std::string &rpc_port,
                                               const AuthFriendReq &req)
{
    AuthFriendRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);
    utils::Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
        return rsp;
    }
    auto &pool = getOrCreatePool(rpc_host, rpc_port);
    ClientContext context;
    auto stub = pool.getConnection();
    Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() { pool.returnConnection(std::move(stub)); });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
    }
    return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &user_info)
{
    (void)base_key;
    (void)uid;
    (void)user_info;
    return true;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(const std::string &rpc_host, const std::string &rpc_port,
                                                 const TextChatMsgReq &req, const Json::Value &root_value)
{
    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);
    utils::Defer defer([&rsp, &req, &root_value]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        for (const auto &text_data : req.textmsgs())
        {
            TextChatData *new_msg = rsp.add_textmsgs();
            new_msg->set_msgid(text_data.msgid());
            new_msg->set_msgcontent(text_data.msgcontent());
        }
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
        return rsp;
    }
    auto &pool = getOrCreatePool(rpc_host, rpc_port);
    ClientContext context;
    auto stub = pool.getConnection();
    Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() { pool.returnConnection(std::move(stub)); });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPCFAILED);
    }
    return rsp;
}
