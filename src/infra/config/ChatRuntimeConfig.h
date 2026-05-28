#pragma once

#include <string>

struct SelfNodeProfile
{
    std::string slot_id;
    std::string name;
    std::string instance_id;
    std::string tcp_bind_host = "0.0.0.0";
    std::string tcp_port;
    std::string rpc_bind_host = "0.0.0.0";
    std::string rpc_port;
    std::string client_advertise_host;
    std::string client_advertise_port;
    std::string rpc_advertise_host;
    std::string rpc_advertise_port;
};

class ChatRuntimeConfig
{
public:
    static ChatRuntimeConfig &getInstance();
    void setSelf(const SelfNodeProfile &node);
    const SelfNodeProfile &self() const;
    bool hasSelf() const;

private:
    ChatRuntimeConfig() = default;
    SelfNodeProfile _self;
    bool _has_self = false;
};
