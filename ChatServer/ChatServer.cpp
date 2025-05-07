// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "LogicSystem.h"
#include <csignal>                      // 用于捕获系统信号（如 Ctrl+C）
#include <thread>                       // C++11 线程库
#include <mutex>                        // 互斥锁
#include "AsioIOServicePool.h"          // IO线程池封装
#include "CServer.h"                    // TCP服务器类
#include "ConfigMgr.h"                  // 配置管理器
#include "RedisMgr.h"                   // Redis 操作封装
#include "ChatServiceImpl.h"           // gRPC 服务实现（聊天服务）

using namespace std;

// 控制服务器退出的标志与同步原语
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
	auto& cfg = ConfigMgr::Inst();                     // 获取配置单例
	auto server_name = cfg["SelfServer"]["Name"];      // 获取当前服务器名，用于 Redis 中标识

	try {
		auto pool = AsioIOServicePool::GetInstance();  // 获取 asio io_context 池的实例（线程池）

		// 将当前服务器的登录连接数设置为 0（用于负载均衡分配）
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

		// 构造 gRPC 服务器地址（例如 127.0.0.1:50055）
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);

		// 实例化聊天服务实现类
		ChatServiceImpl service;
		grpc::ServerBuilder builder;

		// 设置 gRPC 服务监听端口
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

		// 注册服务到 builder 中
		builder.RegisterService(&service);

		// 构建并启动 gRPC 服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		// 启动一个独立线程来执行 gRPC 服务的主循环（阻塞在 Wait）
		std::thread grpc_server_thread([&server]() {
			server->Wait(); // 阻塞，直到 gRPC 服务被 Shutdown
			});

		// 创建一个 asio io_context 用于 TCP 网络服务
		boost::asio::io_context io_context;

		// 注册信号处理器，处理 Ctrl+C 或 kill 信号（优雅退出）
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			// 停止 io_context 和线程池
			io_context.stop();
			pool->Stop();
			// 停止 gRPC 服务
			server->Shutdown();
			});

		// 从配置中读取当前服务器的 TCP 服务端口
		auto port_str = cfg["SelfServer"]["Port"];

		// 创建 TCP Server 并监听客户端连接（用 asio 管理）
		CServer s(io_context, atoi(port_str.c_str()));

		// 启动主事件循环
		io_context.run();

		// 服务退出时，从 Redis 中移除该服务器的登录统计信息
		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();

		// 等待 gRPC 线程结束
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		// 捕获异常时也需要清理资源
		std::cerr << "Exception: " << e.what() << endl;
		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();
	}
}
