// StatusGrpcClient.h - StatusServer RPC 客户端接口
#pragma once

#include "RuntimeContext.h"
#include <optional>
#include <string>

struct UserNodeLocation
{
    std::string node_name;
    std::string rpc_host;
    std::string rpc_port;
};

struct FileServerInfo
{
    std::string host;
    std::string port;
    std::string token;
};

class StatusGrpcClient
{
public:
    virtual ~StatusGrpcClient() = default;

    virtual bool registerNode(const NodeInfo& node) = 0;
    virtual bool unregisterNode(const NodeInfo& node) = 0;
    virtual bool heartbeatNode(const std::string& name, const std::string& instance_id) = 0;
    virtual std::optional<UserNodeLocation> getUserNode(int uid) = 0;
    virtual bool bindUserToNode(int uid, const std::string& node_name) = 0;
    virtual bool unbindUser(int uid) = 0;

    // Token 验证接口
    virtual int validateToken(int uid, const std::string& token) = 0;

    // 刷新登录 token TTL
    virtual bool refreshTokenTTL(int uid) = 0;

    // 通知 StatusServer 用户重新上线，由 StatusServer 转发给 GateServer 刷新 session TTL
    virtual bool notifyUserOnline(int uid) = 0;

    // 获取可用的 FileServer 地址及临时 token
    virtual std::optional<FileServerInfo> getFileServer(int uid) = 0;

    // 删除指定用户的文件传输临时 token
    virtual bool deleteFileToken(int uid) = 0;
};