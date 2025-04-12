#pragma once

#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"

using namespace std;

namespace beast = boost::beast;         // Boost Beast 命名空间
namespace http = beast::http;           // Boost HTTP 模块
namespace net = boost::asio;            // Boost Asio 命名空间
using tcp = boost::asio::ip::tcp;       // TCP 协议

class CServer;      // 前向声明，表示服务器对象
class LogicSystem;  // 前向声明，业务逻辑系统类

// 表示一个 TCP 会话连接，每个连接对应一个客户端
class CSession : public std::enable_shared_from_this<CSession>
{
public:
    // 构造函数，传入 io_context 和服务器指针
    CSession(boost::asio::io_context& io_context, CServer* server);

    // 析构函数
    ~CSession();

    // 获取底层 socket 引用
    tcp::socket& GetSocket();

    // 获取当前 session 的唯一 id
    std::string& GetSessionId();

    // 设置用户 uid（登录成功后绑定）
    void SetUserId(int uid);

    // 获取当前 session 所绑定的 uid
    int GetUserId();

    // 启动会话，开始读取数据
    void Start();

    // 发送数据，低层接口，传入裸数据指针
    void Send(char* msg, short max_length, short msgid);

    // 发送数据，高层接口，传入 string 和消息类型
    void Send(std::string msg, short msgid);

    // 关闭连接
    void Close();

    // 获取 shared_ptr 自身
    std::shared_ptr<CSession> SharedSelf();

    // 异步读取消息体（已知长度）
    void AsyncReadBody(int length);

    // 异步读取消息头（已知消息整体长度）
    void AsyncReadHead(int total_len);

private:
    // 异步读取完整一段内容，绑定一个回调处理器
    void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler);

    // 异步读取指定长度的数据（带偏移）
    void asyncReadLen(std::size_t read_len, std::size_t total_len,
        std::function<void(const boost::system::error_code&, std::size_t)> handler);

    // 写数据回调，用于完成发送
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    tcp::socket _socket;                           // 会话使用的 socket
    std::string _session_id;                       // 会话唯一标识（uuid）
    char _data[MAX_LENGTH];                        // 接收缓冲区
    CServer* _server;                              // 所属服务器指针
    bool _b_close;                                 // 是否已关闭

    std::queue<shared_ptr<SendNode>> _send_que;    // 待发送的消息队列
    std::mutex _send_lock;                         // 发送队列锁

    std::shared_ptr<RecvNode> _recv_msg_node;      // 当前接收到的消息体结构
    bool _b_head_parse;                            // 是否已解析消息头
    std::shared_ptr<MsgNode> _recv_head_node;      // 当前接收到的消息头结构

    int _user_uid;                                 // 当前用户的 uid（登录后绑定）
};

// 表示一个消息 + 会话的逻辑处理对象，由 LogicSystem 统一调度
class LogicNode {
    friend class LogicSystem; // 允许 LogicSystem 访问其私有成员
public:
    // 构造函数，传入 session 和接收到的消息体
    LogicNode(shared_ptr<CSession>, shared_ptr<RecvNode>);
private:
    shared_ptr<CSession> _session;     // 会话指针
    shared_ptr<RecvNode> _recvnode;    // 消息体结构
};
