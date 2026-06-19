// IdGenerator.h - 全局唯一 ID 生成器接口
#pragma once

#include <cstdint>

class IdGenerator
{
public:
    virtual ~IdGenerator() = default;

    virtual uint64_t nextId() = 0;
};
