#pragma once
#include "const.h"


//继承自enable_shared_from_this，用于在异步操作中获取当前对象的shared_ptr,
// 否则在异步操作中使用this会导致对象提前析构
class CServer :public std::enable_shared_from_this<CServer> 
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);
	//启动服务器的异步接收功能，以接受客户端连接请求，并为每个新连接创建一个 HttpConnection 实例进行管理
	void Start();
private:
	tcp::acceptor _acceptor;   //接收器，负责接受对端的连接
	net::io_context& _ioc;     //io_context是一个上下文，是事件的调度器，在底层不断轮询
	//tcp::socket _socket;       
};

