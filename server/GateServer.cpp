﻿#include<iostream>
#include<json/json.h>
#include<json/value.h>
#include<json/reader.h>

#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
	//ConfigMgr gCfgMgr;
	//std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
	//unsigned short gate_port = atoi(gate_port_str.c_str());
	try {
		unsigned short gate_port = static_cast<unsigned short>(8080);
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
			if (error) {
				return;
			}
			ioc.stop();  //捕捉到信号时停止 io_context
		});
		std::make_shared<CServer>(ioc, gate_port)->Start(); //异步监听
		std::cout << "GateServer is listen on port:" << gate_port <<std::endl;
		ioc.run();  //启动事件循环并处理异步操作
	}
	catch (std::exception const& e) {
		std::cerr << "Error:" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}