#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

// 管理多个 io_context 的线程池，支持轮询分发任务
class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>; // 允许 Singleton 调用私有构造函数创建实例

public:
    using IOService = boost::asio::io_context; 
    using Work = boost::asio::io_context::work; // 保持 io_context 不退出
    using WorkPtr = std::unique_ptr<Work>;

    ~AsioIOServicePool(); // 析构函数，清理线程和 io_context

    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete; 

    boost::asio::io_context& GetIOService(); // 轮询获取一个可用的 io_context 用于执行任务
    void Stop(); // 停止所有 io_context 和相关线程

private:
    AsioIOServicePool(std::size_t size = 2); // 初始化指定数量的 io_context 和线程

    std::vector<IOService> _ioServices; // 存储多个 io_context 实例
    std::vector<WorkPtr> _works; // 保持 io_context 存活
    std::vector<std::thread> _threads; // 用于运行 io_context 的线程
    std::size_t _nextIOService; // 记录当前使用的是第几个 io_context（轮询调度）
};
