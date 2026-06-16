// ServiceLocator.h - 轻量级服务定位器
// 提供全局服务注册与获取能力，支持测试时注入 Mock 实现
#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>

/// 轻量级服务定位器
/// 使用 std::any 存储任意类型的服务实例，按接口类型索引
class ServiceLocator
{
public:
    /// 注册服务实例
    /// @tparam Interface 接口类型
    /// @param service 服务实例（通常为 shared_ptr<Interface>）
    template <typename Interface>
    static void registerService(std::shared_ptr<Interface> service)
    {
        _services[std::type_index(typeid(Interface))] = service;
    }

    /// 获取服务实例
    /// @tparam Interface 接口类型
    /// @return 服务实例，未注册时返回 nullptr
    template <typename Interface>
    static std::shared_ptr<Interface> getService()
    {
        auto it = _services.find(std::type_index(typeid(Interface)));
        if (it == _services.end())
        {
            return nullptr;
        }
        return std::any_cast<std::shared_ptr<Interface>>(it->second);
    }

    /// 清空所有服务（测试用）
    static void clear()
    {
        _services.clear();
    }

private:
    inline static std::unordered_map<std::type_index, std::any> _services;
};
