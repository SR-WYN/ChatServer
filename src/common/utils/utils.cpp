// utils.cpp - 通用工具集合实现
#include "utils.h"
#include "Log.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace utils::url
{

static unsigned char toHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

static unsigned char fromHex(unsigned char x)
{
    if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;
    else if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        return x - '0';
    else
        return 0;
}

std::string encode(const std::string &str)
{
    std::string strTemp = "";
    for (unsigned char c : str)
    {
        if (std::isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~'))
        {
            strTemp += c;
        }
        else if (c == ' ')
        {
            strTemp += "%20";
        }
        else
        {
            strTemp += '%';
            strTemp += toHex(c >> 4);
            strTemp += toHex(c & 0x0F);
        }
    }
    return strTemp;
}

std::string decode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
        {
            strTemp += ' ';
        }
        else if (str[i] == '%' && i + 2 < length)
        {
            unsigned char high = fromHex(str[++i]);
            unsigned char low = fromHex(str[++i]);
            strTemp += static_cast<unsigned char>((high << 4) | low);
        }
        else
        {
            strTemp += str[i];
        }
    }
    return strTemp;
}

} // namespace utils::url

namespace utils::env
{

bool isPortAvailable(const std::string &host, const std::string &port_str)
{
    try
    {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor(io);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host),
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

bool envMust(const char *env_key, std::string &value)
{
    if (const char *val = std::getenv(env_key))
    {
        if (val[0] != '\0')
        {
            value = val;
            return true;
        }
    }
    return false;
}

void loadEnvFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        Log::warn(LogModule::Config, "loadEnvFile: cannot open {}", path);
        return;
    }
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        if (!key.empty() && !val.empty())
        {
            setenv(key.c_str(), val.c_str(), 0);
        }
    }
    file.close();
}

std::string makeInstanceUid()
{
    char hostname[256] = {};
    gethostname(hostname, sizeof(hostname) - 1);
    std::ostringstream oss;
    oss << hostname << '-' << getpid();
    return oss.str();
}

} // namespace utils::env

namespace utils::log
{

spdlog::level::level_enum parseLevel(const std::string &level_str)
{
    std::string level = level_str;
    std::transform(level.begin(), level.end(), level.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (level == "trace")
    {
        return spdlog::level::trace;
    }
    if (level == "debug")
    {
        return spdlog::level::debug;
    }
    if (level == "info")
    {
        return spdlog::level::info;
    }
    if (level == "warn" || level == "warning")
    {
        return spdlog::level::warn;
    }
    if (level == "error" || level == "err")
    {
        return spdlog::level::err;
    }
    if (level == "critical" || level == "fatal")
    {
        return spdlog::level::critical;
    }
    if (level == "off")
    {
        return spdlog::level::off;
    }
    return spdlog::level::info;
}

} // namespace utils::log

namespace utils::time
{

std::uint64_t steadyMs()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}

} // namespace utils::time