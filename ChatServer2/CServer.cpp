#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserMgr.h"

// 构造函数：初始化服务器监听对象，并启动接收连接流程
CServer::CServer(boost::asio::io_context& io_context, short port)
    : _io_context(io_context), _port(port),
    _acceptor(io_context, tcp::endpoint(tcp::v4(), port)) // 绑定 IPv4 和端口号
{
    cout << "ChatServer start success, listen on port : " << _port << endl;
    StartAccept(); // 开始异步接收客户端连接
}

// 析构函数：打印服务结束日志
CServer::~CServer() {
    cout << "Server destruct listen on port : " << _port << endl;
}

// 处理客户端连接的回调函数
void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error) {
    if (!error) {
        // 启动 session 的异步读写流程
        new_session->Start();

        // 加锁后将 session 添加到服务器的 session 管理容器中
        lock_guard<mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->GetSessionId(), new_session));
    }
    else {
        // 打印错误信息（一般为网络错误或端口关闭）
        cout << "session accept failed, error is " << error.what() << endl;
    }

    // 不论成功与否，都重新启动下一个异步接收连接（保持服务器持续监听）
    StartAccept();
}

// 启动异步接收新连接
void CServer::StartAccept() {
    // 获取 IO 上下文（多线程的 IO 线程池中获取一个线程）
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();

    // 创建一个新的 session 对象，用于管理即将连接进来的客户端
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);

    // 异步接受连接，绑定处理函数
    _acceptor.async_accept(
        new_session->GetSocket(),
        std::bind(&CServer::HandleAccept, this, new_session, placeholders::_1)
    );
}

// 清除某个已断开的 session 连接
void CServer::ClearSession(std::string uuid) {
    // 如果 session 存在，先解除与用户 ID 的关联
    if (_sessions.find(uuid) != _sessions.end()) {
        // 通知 UserMgr 移除该用户的登录状态（在内存中注销）
        UserMgr::GetInstance()->RmvUserSession(_sessions[uuid]->GetUserId());
    }

    {
        // 加锁后移除 session 对象
        lock_guard<mutex> lock(_mutex);
        _sessions.erase(uuid);
    }
}
