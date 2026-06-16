// LRUCache.h - 线程安全的 LRU 缓存模板
#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>

/// 线程安全的 LRU 缓存
/// @tparam K 键类型
/// @tparam V 值类型
template <typename K, typename V>
class LRUCache
{
public:
    explicit LRUCache(size_t capacity) : _capacity(capacity) {}

    /// 查询缓存，命中时自动提升为最新访问
    std::optional<V> get(const K& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it == _map.end())
        {
            return std::nullopt;
        }
        // 移动到队首（最新访问）
        _list.splice(_list.begin(), _list, it->second);
        return it->second->second;
    }

    /// 插入或更新缓存
    void put(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it != _map.end())
        {
            // 更新已有值并移动到队首
            it->second->second = value;
            _list.splice(_list.begin(), _list, it->second);
            return;
        }

        // 新插入队首
        _list.emplace_front(key, value);
        _map[key] = _list.begin();

        // 超容量淘汰队尾
        if (_map.size() > _capacity)
        {
            auto last = _list.back();
            _map.erase(last.first);
            _list.pop_back();
        }
    }

    /// 删除指定键
    void erase(const K& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it != _map.end())
        {
            _list.erase(it->second);
            _map.erase(it);
        }
    }

    /// 获取当前缓存大小
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _map.size();
    }

    /// 判断缓存是否为空
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _map.empty();
    }

private:
    size_t _capacity;
    std::list<std::pair<K, V>> _list;  // 队首最新，队尾最旧
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> _map;
    mutable std::mutex _mutex;
};
