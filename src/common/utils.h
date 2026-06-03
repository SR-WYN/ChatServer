#pragma once
#include <functional>
#include <string>

namespace utils
{

// 将字节转换为十六进制字符
unsigned char toHex(unsigned char x);

// 将十六进制字符转换为字节
unsigned char fromHex(unsigned char x);

// 对URL进行编码
std::string urlEncode(const std::string& str);

// 对URL进行解码
std::string urlDecode(const std::string& str);

class Defer
{
public:
    explicit Defer(std::function<void()> func);
    ~Defer();
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;

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

} // namespace utils
