#pragma once

#include "RuntimeContext.h"
#include <optional>
#include <string>

struct UserChatLocation
{
    std::string node_name;
    std::string rpc_host;
    std::string rpc_port;
};

// StatusServer 客户端接口：节点注册、用户定位等
class IStatusServiceClient
{
public:
    virtual ~IStatusServiceClient() = default;

    virtual bool registerChatNode(const NodeInfo &node) = 0;
    virtual bool unregisterChatNode(const NodeInfo &node) = 0;
    virtual bool heartbeatChatNode(const std::string &name,
                                   const std::string &instance_id) = 0;
    virtual std::optional<UserChatLocation> getUserChatNode(int uid) = 0;
    virtual bool bindUserToNode(int uid, const std::string &node_name) = 0;
};
