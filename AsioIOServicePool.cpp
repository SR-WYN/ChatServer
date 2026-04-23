#include "AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _io_services(size), _works(size), _next_io_service(0)
{
    for (std::size_t i = 0; i < size; i++)
    {
        _works[i] = std::unique_ptr<Work>(new Work(_io_services[i]));
    }
    // 遍历多个io_service，创建多个线程，每个线程内部启动io_service
    for (std::size_t i = 0; i < _io_services.size(); i++)
    {
        _threads.emplace_back([this, i] {
            _io_services[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool()
{
    stop();
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context &AsioIOServicePool::getIOService()
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto& service = _io_services[_next_io_service++];
    if (_next_io_service == _io_services.size())
    {
        _next_io_service = 0;
    }
    return service;
}

void AsioIOServicePool::stop()
{
    for (auto& work : _works)
    {
        work->get_io_context().stop();
        work.reset();
    }

    for (auto& thread : _threads)
    {
        thread.join();
    }
}