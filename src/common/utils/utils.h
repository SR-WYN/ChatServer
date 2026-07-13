// utils.h - 通用工具集合
#pragma once
#include <cstdint>
#include <spdlog/common.h>
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

namespace utils::log
{

spdlog::level::level_enum parseLevel(const std::string &level_str);

} // namespace utils::log

namespace utils::time
{

int64_t nowSec();
std::uint64_t steadyMs();

} // namespace utils::time