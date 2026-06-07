#pragma once

#include <map>
#include <string>
#include <vector>

// 缓存服务接口：KV / Hash 缓存操作
class ICacheService
{
public:
    virtual ~ICacheService() = default;

    // 基础 KV 操作
    virtual bool get(const std::string &key, std::string &value) = 0;
    virtual bool set(const std::string &key, const std::string &value) = 0;
    virtual bool del(const std::string &key) = 0;

    // Hash 操作
    virtual bool hSet(const std::string &key, const std::string &field,
                      const std::string &value) = 0;
    virtual std::string hGet(const std::string &key, const std::string &field) = 0;
    virtual bool hDel(const std::string &key, const std::string &field) = 0;
    virtual bool hGetAll(const std::string &key,
                         std::map<std::string, std::string> &out) = 0;
};
