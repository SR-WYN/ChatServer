#include "RuntimeContext.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "StatusGrpcClient.h"
#include "utils.h"
#include <sstream>

// 遍历配置槽位，bind 端口后向 StatusServer 注册，注册失败则尝试下一槽位
std::optional<NodeInfo> RuntimeContext::tryRegisterNode()
{
    Log::info(LogModule::Config, "tryRegisterNode: start");
    auto ret = forEachSlot([](const NodeInfo &info) {
        Log::info(LogModule::Config, "tryRegisterNode: trying slot={}", info.slot_key);
        bool ok = StatusGrpcClient::getInstance().registerChatNode(info);
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
std::optional<NodeInfo> RuntimeContext::forEachSlot(
    const std::function<bool(const NodeInfo &)> &accept)
{
    auto &cfg = ConfigMgr::getInstance();
    const std::string slots_csv = cfg["ChatNodePool"]["Slots"];
    if (slots_csv.empty())
    {
        Log::warn(LogModule::Config, "forEachSlot: ChatNodePool.Slots is empty");
        return std::nullopt;
    }

    // 地址必须从环境变量读取
    std::string client_host;
    if (!utils::envMust("CHAT_CLIENT_ADVERTISE_HOST", client_host))
    {
        Log::error(LogModule::Config, "forEachSlot: CHAT_CLIENT_ADVERTISE_HOST not set");
        return std::nullopt;
    }
    std::string rpc_host;
    if (!utils::envMust("CHAT_RPC_ADVERTISE_HOST", rpc_host))
    {
        Log::error(LogModule::Config, "forEachSlot: CHAT_RPC_ADVERTISE_HOST not set");
        return std::nullopt;
    }

    const std::string instance_uid = utils::makeInstanceUid();

    Log::info(LogModule::Config, "forEachSlot: slots=[{}] client_host={} rpc_host={} uid={}",
              slots_csv, client_host, rpc_host, instance_uid);

    std::stringstream ss(slots_csv);
    std::string slot_key;
    while (std::getline(ss, slot_key, ','))
    {
        if (slot_key.empty())
        {
            continue;
        }

        // 端口必须从环境变量读取
        std::string tcp_env_key = "CHAT_SLOT" + slot_key + "_TCP_PORT";
        std::string rpc_env_key = "CHAT_SLOT" + slot_key + "_RPC_PORT";
        std::string tcp_port, rpc_port;
        if (!utils::envMust(tcp_env_key.c_str(), tcp_port) ||
            !utils::envMust(rpc_env_key.c_str(), rpc_port))
        {
            Log::warn(LogModule::Config, "forEachSlot: slot={} missing env {}/{}",
                      slot_key, tcp_env_key, rpc_env_key);
            continue;
        }

        // 端口已被占用则跳过
        if (!utils::isPortAvailable("0.0.0.0", tcp_port))
        {
            Log::warn(LogModule::Config, "forEachSlot: slot={} tcp port {} unavailable", slot_key, tcp_port);
            continue;
        }
        if (!utils::isPortAvailable("0.0.0.0", rpc_port))
        {
            Log::warn(LogModule::Config, "forEachSlot: slot={} rpc port {} unavailable", slot_key, rpc_port);
            continue;
        }

        NodeInfo info;
        info.slot_key              = slot_key;
        info.name                  = "ChatNode-" + slot_key;
        info.instance_uid          = instance_uid;
        info.tcp_port              = tcp_port;
        info.rpc_port              = rpc_port;
        info.client_advertise_host = client_host;
        info.client_advertise_port = tcp_port;
        info.rpc_advertise_host    = rpc_host;
        info.rpc_advertise_port    = rpc_port;

        Log::info(LogModule::Config, "forEachSlot: slot={} ports=[tcp={},rpc={}] trying",
                  slot_key, tcp_port, rpc_port);

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
    self_info_ = node;
    is_initialized_ = true;
    Log::info(LogModule::Config, "setNodeInfo: slot={} name={} uid={}",
              node.slot_key, node.name, node.instance_uid);
}

// 获取当前节点身份信息
const NodeInfo &RuntimeContext::getNodeInfo() const
{
    return self_info_;
}

// 当前节点是否已初始化
bool RuntimeContext::isInitialized() const
{
    return is_initialized_;
}
