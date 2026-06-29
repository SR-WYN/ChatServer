// SnowflakeIdGenerator.h - 雪花 ID 生成器实现
#pragma once

#include "IdGenerator.h"
#include "snowflake_id.h"
#include <memory>

class SnowflakeIdGenerator final : public IdGenerator
{
public:
    explicit SnowflakeIdGenerator(uint64_t node_id);
    ~SnowflakeIdGenerator() override = default;

    uint64_t nextId() override;

private:
    std::unique_ptr<utils::SnowflakeId> _snowflake;
};
