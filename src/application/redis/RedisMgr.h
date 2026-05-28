#pragma once

#include "Singleton.h"
#include <cstddef>
#include <string>

class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();
    bool get(const std::string &key, std::string &value);
    bool set(const std::string &key, const std::string &value);
    bool hSet(const char *key, const char *field, const char *val, size_t vallen);
    bool hSet(const std::string &key, const std::string &field, const std::string &value);
    bool hDel(const std::string &key, const std::string &field);
    void close();
    std::string hGet(const std::string &key, const std::string &field);

private:
    RedisMgr();
    RedisMgr(const RedisMgr &src) = delete;
    RedisMgr &operator=(const RedisMgr &src) = delete;
};
