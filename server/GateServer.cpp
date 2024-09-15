#include<iostream>
#include<json/json.h>
#include<json/value.h>
#include<json/reader.h>

#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
  
    ConfigMgr gCfgMgr; //读取配置文件类
    std::string gate_port_str = gCfgMgr["GateServer"]["Port"];


    // 将字符串端口号转换为 unsigned short 类型，作为后续监听使用的端口
    unsigned short gate_port = atoi(gate_port_str.c_str());

    try {
        // 创建 io_context 对象，用于管理异步 I/O 操作
        net::io_context ioc{ 1 };
        //1 表示单线程模式：所有异步事件将由单个线程处理。这意味着所有异步 I/O 操作将在同一个线程上依次执行，
        // 适合简单的应用场景或不需要多线程处理的任务。

       /* 如果指定更大的数字，比如 2 或更多，那么 io_context.run() 可以在多个线程中并行执行，
            以提高性能并利用多核 CPU 的能力。*/


        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

        // 异步等待信号，当捕捉到信号时执行回调函数
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            // 如果捕捉信号时发生错误，则直接返回，不执行后续操作
            if (error) {
                return;
            }
            // 当捕捉到指定信号时，停止 io_context 对象的运行（停止事件循环）
            ioc.stop();  // 捕捉到信号时停止 io_context
            });

        // 创建 CServer 对象并启动服务器，监听指定端口，进行异步操作
        std::make_shared<CServer>(ioc, gate_port)->Start(); // 调用async_accept异步监听
        //异步操作只是向 io_context 注册了一个等待执行的操作，它不会立刻执行，
        // 真正执行这些异步操作的过程是通过事件循环 ioc.run() 来驱动的。

        std::cout << "GateServer is listening on port:" << gate_port << std::endl;

        // 开始运行 io_context 的事件循环，处理异步操作
        ioc.run();  // 启动事件循环并处理异步操作
    }
    catch (std::exception const& e) {
        // 捕捉所有异常，并输出错误信息
        std::cerr << "Error:" << e.what() << std::endl;
        return EXIT_FAILURE; // 如果发生异常，返回失败状态码
    }
}
