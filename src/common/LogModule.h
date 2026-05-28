#pragma once

#include <array>
#include <cstddef>
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

namespace LogNames
{
inline constexpr std::string_view _app = "app";
inline constexpr std::string_view _config = "config";
inline constexpr std::string_view _net = "net";
inline constexpr std::string_view _mysql = "mysql";
inline constexpr std::string_view _redis = "redis";
inline constexpr std::string_view _grpc = "grpc";
inline constexpr std::string_view _logic = "logic";
inline constexpr std::string_view _login = "login";
inline constexpr std::string_view _user = "user";
inline constexpr std::string_view _friend = "friend";
inline constexpr std::string_view _chat = "chat";

inline constexpr std::array<std::string_view, 11> _table = {
    _app, _config, _net, _mysql, _redis, _grpc, _logic, _login, _user, _friend, _chat,
};
} // namespace LogNames

inline std::string_view moduleName(LogModule module)
{
    return LogNames::_table[static_cast<std::size_t>(module)];
}
