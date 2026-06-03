#pragma once

#include "Singleton.h"
#include <functional>
#include <optional>
#include <string>

// 当前 ChatNode 的运行时身份信息
struct NodeInfo
{
    std::string slot_key;               // 配置槽位标识
    std::string name;                   // 节点名
    std::string instance_uid;           // 实例唯一标识
    std::string tcp_host = "0.0.0.0";
    std::string tcp_port;
    std::string rpc_host = "0.0.0.0";
    std::string rpc_port;
    std::string client_advertise_host;  // 对客户端公布的地址
    std::string client_advertise_port;
    std::string rpc_advertise_host;     // 对 RPC 对端公布的地址
    std::string rpc_advertise_port;
};

// 运行时上下文单例
// 职责：
//   1. 扫描配置槽位、尝试 bind 端口、向 StatusServer 注册
//   2. 持有当前节点的 NodeInfo，供全局查询
class RuntimeContext : public Singleton<RuntimeContext>
{
    friend class Singleton<RuntimeContext>;

public:
    // 遍历配置槽位，尝试 bind TCP/RPC 端口并向 StatusServer 注册；
    // 注册失败则尝试下一槽位
    static std::optional<NodeInfo> tryRegisterNode();

    // 设置当前节点身份信息
    void setNodeInfo(const NodeInfo &node);

    // 获取当前节点身份信息
    const NodeInfo &getNodeInfo() const;

    // 当前节点是否已初始化
    bool isInitialized() const;

private:
    RuntimeContext() = default;

    // 遍历配置槽位，bind 端口后回调 accept，接受则返回该 NodeInfo
    static std::optional<NodeInfo> forEachSlot(
        const std::function<bool(const NodeInfo &)> &accept);

    NodeInfo self_info_;
    bool is_initialized_ = false;
};
