#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>
#include <mutex>

using namespace std;
using boost::asio::ip::tcp;

// CServer 是基于 Boost.Asio 实现的 TCP 服务端类
// 用于监听客户端连接并管理多个会话 CSession 对象
class CServer
{
public:
    // 构造函数：初始化 io_context 和监听端口
    // 创建 acceptor 监听 TCP 连接
    CServer(boost::asio::io_context& io_context, short port);

    // 析构函数：做清理工作
    ~CServer();

    // 从会话管理表中移除某个 Session（比如断开连接时调用）
    void ClearSession(std::string session_id);

private:
    // 处理新连接的回调函数（当有客户端连接进来时自动调用）
    void HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error);

    // 开始异步接收下一个客户端连接
    void StartAccept();

    // Boost 的主 IO 上下文，用于管理异步事件循环
    boost::asio::io_context& _io_context;

    // 服务端监听的端口号
    short _port;

    // TCP 接收器，用于接收连接请求
    tcp::acceptor _acceptor;

    // 当前所有活跃连接的 Session 列表，key 是 Session ID
    std::map<std::string, shared_ptr<CSession>> _sessions;

    // 用于保护 sessions 列表的互斥锁，防止多线程访问冲突
    std::mutex _mutex;
};
