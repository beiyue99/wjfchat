#pragma once
#include "const.h"

// 表示一个 HTTP 连接，负责解析请求和返回响应
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem; // 允许 LogicSystem 访问私有成员

public:
    HttpConnection(boost::asio::io_context& ioc); // 构造函数，初始化 socket

    void Start(); // 启动连接处理，异步读取请求数据
    void PreParseGetParam(); // 预处理 GET 请求的参数
    tcp::socket& GetSocket() { return _socket; } // 获取当前连接的 socket

private:
    void CheckDeadline(); // 检查连接是否超时并关闭
    void WriteResponse(); // 写入并发送 HTTP 响应
    void HandleReq(); // 处理 HTTP 请求逻辑

    tcp::socket  _socket; // 与客户端通信的 socket

    beast::flat_buffer  _buffer{ 8192 }; // 用于接收数据的缓冲区

    http::request<http::dynamic_body> _request; // 存储客户端请求
    http::response<http::dynamic_body> _response; // 构造服务端响应

    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60)
    }; // 定时器，用于检测连接是否超时

    std::string _get_url; // GET 请求中的 URL
    std::unordered_map<std::string, std::string> _get_params; // 存储 GET 请求的参数
};
