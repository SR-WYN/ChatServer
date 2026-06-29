// snowflake_id.cpp - 雪花 ID 生成器实现
#include "snowflake_id.h"
#include <chrono>
#include <thread>

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

    if (now < _last_ms)
    {
        now = waitNextMs(_last_ms);
    }

    if (now == _last_ms)
    {
        _sequence = (_sequence + 1) & kMaxSequence;
        if (_sequence == 0)
        {
            now = waitNextMs(_last_ms);
        }
    }
    else
    {
        _sequence = 0;
    }

    _last_ms = now;

    return (now << kTimestampShift) | (_node_id << kNodeIdShift) | _sequence;
}