#pragma once
#include "const.h"


class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);
		//io_context是一个上下文，是事件的调度器，在底层不断轮询


	//启动服务器的异步接收功能，以接受客户端连接请求，并为每个新连接创建一个 HttpConnection 实例进行管理
	void Start();
private:
	tcp::acceptor _acceptor;   //接收器，负责接受对端的连接
	net::io_context& _ioc;      //上下文
	//tcp::socket _socket;       
};

