#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket);
	void Start();  //用于监听读写事件
private:
	void CheckDeadline();   //超时检测
	void WriteResponse();  //收到数据后的应答函数
	void HandleReq(); //处理请求
	void PreParseGetParam();// 
	请求的参数解析
	tcp::socket _socket;
	beast::flat_buffer _buffer{ 8192 }; //接收数据的buffer
	http::request<http::dynamic_body> _request; //接收对方的请求
	http::response<http::dynamic_body> _response; //回复对方
	net::steady_timer deadline_{
		_socket.get_executor(),std::chrono::seconds(60)//60秒超时 
	//定时器在底层事件循环，需要调度器
	};
	std::string _get_url;   //请求url
	std::unordered_map<std::string, std::string> _get_params;   
};

