#pragma once

#include <string_view>

enum class LogModule
{
    App,
    Config,
    Net,
    Mysql,
    Redis,
    Grpc,
    Logic,
    Login,
    User,
    Friend,
    Chat,
};

inline std::string_view moduleName(LogModule module)
{
    switch (module)
    {
    case LogModule::App:
        return "app";
    case LogModule::Config:
        return "config";
    case LogModule::Net:
        return "net";
    case LogModule::Mysql:
        return "mysql";
    case LogModule::Redis:
        return "redis";
    case LogModule::Grpc:
        return "grpc";
    case LogModule::Logic:
        return "logic";
    case LogModule::Login:
        return "login";
    case LogModule::User:
        return "user";
    case LogModule::Friend:
        return "friend";
    case LogModule::Chat:
        return "chat";
    }
    return "unknown";
}
