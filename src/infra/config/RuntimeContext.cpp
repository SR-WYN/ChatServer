#include "RuntimeContext.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "StatusGrpcClient.h"
#include "utils.h"
#include <sstream>

// 遍历配置槽位，bind 端口后向 StatusServer 注册，注册失败则尝试下一槽位
std::optional<NodeInfo> RuntimeContext::tryRegisterNode(StatusGrpcClient* client)
{
    Log::info(LogModule::Config, "tryRegisterNode: start");
    auto ret = forEachSlot([client](const NodeInfo &info) {
        Log::info(LogModule::Config, "tryRegisterNode: trying slot={}", info.slot_key);
        bool ok = client && client->registerChatNode(info);
        if (ok)
        {
            Log::info(LogModule::Config, "tryRegisterNode: slot={} registered", info.slot_key);
        }
        else
        {
            Log::warn(LogModule::Config, "tryRegisterNode: slot={} register failed", info.slot_key);
        }
        return ok;
    });
    if (ret)
    {
        Log::info(LogModule::Config, "tryRegisterNode: acquired slot={}", ret->slot_key);
    }
    else
    {
        Log::warn(LogModule::Config, "tryRegisterNode: no available slot");
    }
    return ret;
}

// 遍历配置槽位列表，bind 端口后回调 accept，接受则返回该 NodeInfo
std::optional<NodeInfo>
RuntimeContext::forEachSlot(const std::function<bool(const NodeInfo &)> &accept)
{
    auto &cfg = ConfigMgr::getInstance();
    const std::string slots_csv = cfg["ChatNodePool"]["Slots"];
    if (slots_csv.empty())
    {
        Log::warn(LogModule::Config, "forEachSlot: ChatNodePool.Slots is empty");
        return std::nullopt;
    }

    const std::string instance_uid = utils::makeInstanceUid();

    Log::info(LogModule::Config, "forEachSlot: slots=[{}] uid={}", slots_csv, instance_uid);

    std::stringstream ss(slots_csv);
    std::string slot_key;
    while (std::getline(ss, slot_key, ','))
    {
        if (slot_key.empty())
        {
            continue;
        }

        // 从 config.json 的 ChatNode_{slot_key} 节中读取节点配置
        std::string tcp_port, rpc_port, client_host, rpc_host;
        bool found = false;

        std::string node_section = "ChatNode_" + slot_key;
        auto section = cfg[node_section];
        tcp_port = section["TcpPort"];
        rpc_port = section["RpcPort"];
        client_host = section["ClientAdvertiseHost"];
        rpc_host = section["RpcAdvertiseHost"];

        if (!tcp_port.empty() && !rpc_port.empty() && !client_host.empty() && !rpc_host.empty())
        {
            found = true;
        }

        if (!found)
        {
            Log::warn(LogModule::Config,
                      "forEachSlot: slot={} config not found in ChatNodePool.Nodes", slot_key);
            continue;
        }

        // 端口已被占用则跳过
        if (!utils::isPortAvailable("0.0.0.0", tcp_port))
        {
            Log::warn(LogModule::Config, "forEachSlot: slot={} tcp port {} unavailable", slot_key,
                      tcp_port);
            continue;
        }
        if (!utils::isPortAvailable("0.0.0.0", rpc_port))
        {
            Log::warn(LogModule::Config, "forEachSlot: slot={} rpc port {} unavailable", slot_key,
                      rpc_port);
            continue;
        }

        NodeInfo info;
        info.slot_key = slot_key;
        info.name = "ChatNode-" + slot_key;
        info.instance_uid = instance_uid;
        info.tcp_port = tcp_port;
        info.rpc_port = rpc_port;
        info.client_advertise_host = client_host;
        info.client_advertise_port = tcp_port;
        info.rpc_advertise_host = rpc_host;
        info.rpc_advertise_port = rpc_port;

        Log::info(LogModule::Config,
                  "forEachSlot: slot={} ports=[tcp={},rpc={}] client_host={} rpc_host={} trying",
                  slot_key, tcp_port, rpc_port, client_host, rpc_host);

        if (accept(info))
        {
            Log::info(LogModule::Config, "forEachSlot: slot={} accepted", slot_key);
            return info;
        }
    }

    Log::warn(LogModule::Config, "forEachSlot: all slots exhausted");
    return std::nullopt;
}

// 设置当前节点身份信息
void RuntimeContext::setNodeInfo(const NodeInfo &node)
{
    _self_info = node;
    _is_initialized = true;
    Log::info(LogModule::Config, "setNodeInfo: slot={} name={} uid={}", node.slot_key, node.name,
              node.instance_uid);
}

// 获取当前节点身份信息
const NodeInfo &RuntimeContext::getNodeInfo() const
{
    return _self_info;
}

// 当前节点是否已初始化
bool RuntimeContext::isInitialized() const
{
    return _is_initialized;
}
