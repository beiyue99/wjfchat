#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"





//io_context 本质上维护一个任务队列（Task Queue）。当你将 I/O 操作（例如异步 socket 操作）提交给 io_context 时，
// 任务会被放入队列中。这些任务可以是网络 I / O 操作、定时器操作、或者其他用户自定义的任务。
//每次调用 io_context::run()，它会不断从队列中取出任务并执行，直到所有任务执行完毕。
//io_context 会监听这些操作的完成状态。当某个异步操作完成时，io_context 会将对应的回调函数放入任务队列






class AsioIOServicePool :public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t                        _nextIOService;
};

