// LRUCache.h - 高性能 LRU 缓存模板
#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>

/// 高性能 LRU 缓存
/// @tparam K 键类型
/// @tparam V 值类型
/// @tparam DEFAULT_CAPACITY 默认容量上限（10000）
template <typename K, typename V, size_t DEFAULT_CAPACITY = 10000>
class LRUCache
{
public:
    explicit LRUCache(size_t capacity) : _capacity(capacity), _size(0) {}

    LRUCache() : _capacity(DEFAULT_CAPACITY), _size(0) {}

    /// 查询缓存，命中时自动提升为最新访问
    std::optional<V> get(const K& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it == _map.end())
        {
            return std::nullopt;
        }
        _list.splice(_list.begin(), _list, it->second);
        return it->second->second;
    }

    /// 查询缓存，命中但不提升访问顺序
    std::optional<V> peek(const K& key) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it == _map.end())
        {
            return std::nullopt;
        }
        return it->second->second;
    }

    /// 插入或更新缓存
    void put(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it != _map.end())
        {
            it->second->second = value;
            _list.splice(_list.begin(), _list, it->second);
            return;
        }

        _list.emplace_front(key, value);
        _map[key] = _list.begin();
        _size++;

        if (_size > _capacity)
        {
            evict();
        }
    }

    /// 就地构造缓存条目（避免拷贝）
    template <typename... Args>
    void emplace(const K& key, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _map.find(key);
        if (it != _map.end())
        {
            it->second->second = V{std::forward<Args>(args)...};
            _list.splice(_list.begin(), _list, it->second);
            return;
        }

        _list.emplace_front(key, std::forward<Args>(args)...);
        _map[key] = _list.begin();
        _size++;

        if (_size > _capacity)
        {
            evict();
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
            _size--;
        }
    }

    /// 清空所有条目
    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _list.clear();
        _map.clear();
        _size = 0;
    }

    /// 获取当前缓存大小（无锁，原子读取）
    size_t size() const { return _size.load(); }

    /// 判断缓存是否为空（无锁，原子读取）
    bool empty() const { return _size.load() == 0; }

private:
    void evict()
    {
        auto last = _list.back();
        _map.erase(last.first);
        _list.pop_back();
        _size--;
    }

    size_t _capacity;
    std::atomic<size_t> _size;
    std::list<std::pair<K, V>> _list;  // 队首最新，队尾最旧
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> _map;
    mutable std::mutex _mutex;
};
