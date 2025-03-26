#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"



class AsioIOServicePool :public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>; // 声明为友元，以便调用私有构造函数
public:
    using IOService = boost::asio::io_context; 
	using Work = boost::asio::io_context::work; // 用于防止 io_service 对象在没有工作时停止
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool(); 
    AsioIOServicePool(const AsioIOServicePool&) = delete; 
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete; 
	boost::asio::io_context& GetIOService();  // 获取一个 io_service 对象
    void Stop();
private:
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
    std::vector<IOService> _ioServices; 
    std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;   // 用于运行 io_service 的线程
	std::size_t _nextIOService;   // 下一个 io_service 的索引
};

