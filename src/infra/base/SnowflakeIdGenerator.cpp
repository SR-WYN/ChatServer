// SnowflakeIdGenerator.cpp
#include "SnowflakeIdGenerator.h"

SnowflakeIdGenerator::SnowflakeIdGenerator(uint64_t node_id)
    : _snowflake(std::make_unique<utils::SnowflakeId>(node_id))
{
}

uint64_t SnowflakeIdGenerator::nextId()
{
    return _snowflake->nextId();
}
