#include "ChatNodeSlotSelector.h"
#include "ConfigMgr.h"
#include "StatusGrpcClient.h"
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace
{
bool tryBindPort(const std::string &host, const std::string &port_str)
{
    try
    {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor(io);
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::make_address(host),
            static_cast<unsigned short>(std::stoi(port_str)));
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen(boost::asio::socket_base::max_listen_connections);
        acceptor.close();
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

std::string envOrDefault(const char *env_key, const std::string &fallback)
{
    if (const char *val = std::getenv(env_key))
    {
        if (val[0] != '\0')
        {
            return val;
        }
    }
    return fallback;
}

std::string makeInstanceId()
{
    char hostname[256] = {};
    gethostname(hostname, sizeof(hostname) - 1);
    std::ostringstream oss;
    oss << hostname << '-' << getpid();
    return oss.str();
}
} // namespace

std::optional<SelfNodeProfile> ChatNodeSlotSelector::acquire()
{
    auto &cfg = ConfigMgr::getInstance();
    const std::string slots_csv = cfg["ChatNodePool"]["Slots"];
    if (slots_csv.empty())
    {
        return std::nullopt;
    }

    const std::string default_client_host = cfg["ChatNodePool"]["ClientAdvertiseHost"];
    const std::string default_rpc_host = cfg["ChatNodePool"]["RpcAdvertiseHost"];
    const std::string client_host =
        envOrDefault("CHAT_CLIENT_ADVERTISE_HOST", default_client_host);
    const std::string rpc_host = envOrDefault("CHAT_RPC_ADVERTISE_HOST", default_rpc_host);
    const std::string instance_id = makeInstanceId();

    std::stringstream ss(slots_csv);
    std::string slot_id;
    while (std::getline(ss, slot_id, ','))
    {
        if (slot_id.empty())
        {
            continue;
        }
        auto slot_cfg = cfg[slot_id];
        const std::string tcp_port = slot_cfg["TcpPort"];
        const std::string rpc_port = slot_cfg["RpcPort"];
        if (tcp_port.empty() || rpc_port.empty())
        {
            continue;
        }
        if (!tryBindPort("0.0.0.0", tcp_port) || !tryBindPort("0.0.0.0", rpc_port))
        {
            continue;
        }

        SelfNodeProfile profile;
        profile.slot_id = slot_id;
        profile.name = "ChatNode-" + slot_id;
        profile.instance_id = instance_id;
        profile.tcp_port = tcp_port;
        profile.rpc_port = rpc_port;
        profile.client_advertise_host = client_host;
        profile.client_advertise_port = tcp_port;
        profile.rpc_advertise_host = rpc_host;
        profile.rpc_advertise_port = rpc_port;
        return profile;
    }

    return std::nullopt;
}

std::optional<SelfNodeProfile> ChatNodeSlotSelector::acquireAndRegister()
{
    auto &cfg = ConfigMgr::getInstance();
    const std::string slots_csv = cfg["ChatNodePool"]["Slots"];
    if (slots_csv.empty())
    {
        return std::nullopt;
    }

    const std::string default_client_host = cfg["ChatNodePool"]["ClientAdvertiseHost"];
    const std::string default_rpc_host = cfg["ChatNodePool"]["RpcAdvertiseHost"];
    const std::string client_host =
        envOrDefault("CHAT_CLIENT_ADVERTISE_HOST", default_client_host);
    const std::string rpc_host = envOrDefault("CHAT_RPC_ADVERTISE_HOST", default_rpc_host);
    const std::string instance_id = makeInstanceId();

    std::stringstream ss(slots_csv);
    std::string slot_id;
    while (std::getline(ss, slot_id, ','))
    {
        if (slot_id.empty())
        {
            continue;
        }
        auto slot_cfg = cfg[slot_id];
        const std::string tcp_port = slot_cfg["TcpPort"];
        const std::string rpc_port = slot_cfg["RpcPort"];
        if (tcp_port.empty() || rpc_port.empty())
        {
            continue;
        }
        if (!tryBindPort("0.0.0.0", tcp_port) || !tryBindPort("0.0.0.0", rpc_port))
        {
            continue;
        }

        SelfNodeProfile profile;
        profile.slot_id = slot_id;
        profile.name = "ChatNode-" + slot_id;
        profile.instance_id = instance_id;
        profile.tcp_port = tcp_port;
        profile.rpc_port = rpc_port;
        profile.client_advertise_host = client_host;
        profile.client_advertise_port = tcp_port;
        profile.rpc_advertise_host = rpc_host;
        profile.rpc_advertise_port = rpc_port;

        if (StatusGrpcClient::getInstance().registerChatNode(profile))
        {
            return profile;
        }
    }

    return std::nullopt;
}
