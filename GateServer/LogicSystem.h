#pragma once
#include "const.h"




class HttpConnection;
//定义个可调用对象HttpHandler   返回值为void，参数为一个http连接对象
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;


class LogicSystem :public Singleton<LogicSystem> {
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	//接收到一个HTTP GET 请求时，根据请求的路径 (path) 查找并调用相应的处理器函数。
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	//接收到一个HTTP POST 请求时，根据请求的路径 (path) 查找并调用相应的处理器函数。
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	//注册一个用于处理特定 URL 路径的 HTTP GET 请求处理器
	void RegGet(std::string, HttpHandler handler);
	//注册一个用于处理特定 URL 路径的 HTTP POST 请求处理器
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	//注册处理HTTP GET 请求，解析 URL 参数并返回给客户端。
	//注册处理HTTP POST 请求，解析消息体中的 JSON 数据，并根据内容生成相应的响应。
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};


