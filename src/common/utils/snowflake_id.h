// snowflake_id.h - 雪花 ID 生成器（线程安全）
// 64 位结构：0 | 41 bits 时间戳 | 10 bits 节点ID | 12 bits 序列号
#pragma once
#include <cstdint>
#include <mutex>

namespace utils
{

class SnowflakeId
{
public:
    static constexpr uint64_t kEpoch = 1735689600000ULL;
    static constexpr uint64_t kMaxNodeId = (1ULL << 10) - 1;
    static constexpr uint64_t kMaxSequence = (1ULL << 12) - 1;
    static constexpr uint64_t kNodeIdShift = 12;
    static constexpr uint64_t kTimestampShift = 22;

    explicit SnowflakeId(uint64_t node_id);
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