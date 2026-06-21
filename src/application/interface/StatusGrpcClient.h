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

    // 新增：Token 验证接口
    virtual int validateToken(int uid, const std::string& token) = 0;
};