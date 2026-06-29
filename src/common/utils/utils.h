#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace utils
{

// ---- 编解码 ----

/** 将字节转换为十六进制字符 */
unsigned char toHex(unsigned char x);

/** 将十六进制字符转换为字节 */
unsigned char fromHex(unsigned char x);

/** 对URL进行编码 */
std::string urlEncode(const std::string &str);

/** 对URL进行解码 */
std::string urlDecode(const std::string &str);

// ---- RAII ----

class Defer
{
public:
    explicit Defer(std::function<void()> func);
    ~Defer();
    Defer(const Defer &) = delete;
    Defer &operator=(const Defer &) = delete;

private:
    std::function<void()> _func;
};

// ---- 系统/网络工具 ----

/** 尝试 bind 指定端口，检测端口是否可用 */
bool isPortAvailable(const std::string &host, const std::string &port_str);

/** 读取环境变量，若为空或不存在则返回默认值 */
std::string envOrDefault(const char *env_key, const std::string &fallback);

/** 读取环境变量，若为空或不存在则返回 false，value 接收结果 */
bool envMust(const char *env_key, std::string &value);

/** 加载 .env 文件中的环境变量到进程环境 */
void loadEnvFile(const std::string &path);

/** 生成实例唯一标识：hostname-pid */
std::string makeInstanceUid();

// ---- 雪花 ID 生成器 ----

/**
 * 雪花 ID 生成器（线程安全）
 * 64 位结构：0 | 41 bits 时间戳 | 10 bits 节点ID | 12 bits 序列号
 * 自定义纪元：2025-01-01 00:00:00 UTC
 */
class SnowflakeId
{
public:
    static constexpr uint64_t kEpoch = 1735689600000ULL;       // 自定义纪元毫秒
    static constexpr uint64_t kMaxNodeId = (1ULL << 10) - 1;   // 1023
    static constexpr uint64_t kMaxSequence = (1ULL << 12) - 1; // 4095
    static constexpr uint64_t kNodeIdShift = 12;
    static constexpr uint64_t kTimestampShift = 22;

    /** 构造 SnowflakeId 生成器，node_id: 节点 ID（0 ~ 1023） */
    explicit SnowflakeId(uint64_t node_id);

    /** 生成下一个唯一 ID（线程安全） */
    uint64_t nextId();

private:
    uint64_t nowMs() const;
    uint64_t waitNextMs(uint64_t last_ms) const;

    uint64_t _node_id;
    uint64_t _last_ms = 0;
    uint64_t _sequence = 0;
    std::mutex _mutex;
};

} // namespace utils
