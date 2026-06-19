#include "utils.h"
#include "Log.h"
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

// ---- 编解码 ----

unsigned char utils::toHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

unsigned char utils::fromHex(unsigned char x)
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

std::string utils::urlEncode(const std::string &str)
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
            strTemp += toHex(c >> 4);   // 取高4位
            strTemp += toHex(c & 0x0F); // 取低4位
        }
    }
    return strTemp;
}

std::string utils::urlDecode(const std::string &str)
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

// ---- RAII ----

utils::Defer::Defer(std::function<void()> func) : _func(func)
{
}

utils::Defer::~Defer()
{
    if (_func)
        _func();
}

// ---- 系统/网络工具 ----

bool utils::isPortAvailable(const std::string &host, const std::string &port_str)
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

std::string utils::envOrDefault(const char *env_key, const std::string &fallback)
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

bool utils::envMust(const char *env_key, std::string &value)
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

void utils::loadEnvFile(const std::string &path)
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
        // 跳过空行和注释
        if (line.empty() || line[0] == '#')
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        if (!key.empty() && !val.empty())
        {
            setenv(key.c_str(), val.c_str(), 0); // 0 = 不覆盖已有变量
        }
    }
    file.close();
}

std::string utils::makeInstanceUid()
{
    char hostname[256] = {};
    gethostname(hostname, sizeof(hostname) - 1);
    std::ostringstream oss;
    oss << hostname << '-' << getpid();
    return oss.str();
}

// ---- 雪花 ID 生成器 ----

utils::SnowflakeId::SnowflakeId(uint64_t node_id) : _node_id(node_id & kMaxNodeId)
{
}

uint64_t utils::SnowflakeId::nowMs() const
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<uint64_t>(ms) - kEpoch;
}

uint64_t utils::SnowflakeId::waitNextMs(uint64_t last_ms) const
{
    uint64_t cur = nowMs();
    while (cur <= last_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        cur = nowMs();
    }
    return cur;
}

uint64_t utils::SnowflakeId::nextId()
{
    std::lock_guard<std::mutex> lock(_mutex);

    uint64_t now = nowMs();

    // 时钟回拨：等待到下一毫秒
    if (now < _last_ms)
    {
        now = waitNextMs(_last_ms);
    }

    // 同一毫秒内，序列号递增
    if (now == _last_ms)
    {
        _sequence = (_sequence + 1) & kMaxSequence;
        // 序列号耗尽，等待下一毫秒
        if (_sequence == 0)
        {
            now = waitNextMs(_last_ms);
        }
    }
    else
    {
        // 新的一毫秒，序列号重置
        _sequence = 0;
    }

    _last_ms = now;

    // 组装 64 位 ID
    return (now << kTimestampShift) | (_node_id << kNodeIdShift) | _sequence;
}
