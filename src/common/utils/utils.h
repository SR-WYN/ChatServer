// utils.h - 通用工具集合
#pragma once
#include <string>

namespace utils::url
{

std::string encode(const std::string &str);
std::string decode(const std::string &str);

} // namespace utils::url

namespace utils::env
{

bool isPortAvailable(const std::string &host, const std::string &port_str);
std::string envOrDefault(const char *env_key, const std::string &fallback);
bool envMust(const char *env_key, std::string &value);
void loadEnvFile(const std::string &path);
std::string makeInstanceUid();

} // namespace utils::env