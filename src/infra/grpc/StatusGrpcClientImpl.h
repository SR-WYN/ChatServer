// StatusGrpcClientImpl.h - StatusServer RPC 客户端实现
#pragma once

#include "StatusGrpcClient.h"
#include <memory>

class StatusConPool;

class StatusGrpcClientImpl final : public StatusGrpcClient
{
public:
    StatusGrpcClientImpl();
    ~StatusGrpcClientImpl() override;

    bool registerNode(const NodeInfo& node) override;
    bool unregisterNode(const NodeInfo& node) override;
    bool heartbeatNode(const std::string& name, const std::string& instance_id) override;
    std::optional<UserNodeLocation> getUserNode(int uid) override;
    bool bindUserToNode(int uid, const std::string& node_name) override;
    bool unbindUser(int uid) override;
    int validateToken(int uid, const std::string& token) override;

private:
    std::unique_ptr<StatusConPool> _pool;
};
