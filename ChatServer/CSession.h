#pragma once  
#include <boost/asio.hpp>  
#include <boost/uuid/uuid_io.hpp>  
#include <boost/uuid/uuid_generators.hpp>  
#include <boost/beast/http.hpp>  
#include <boost/beast.hpp>  
#include <queue>  
#include <mutex>     
#include <memory>    
#include "const.h"   
#include "MsgNode.h" 

using namespace std;

namespace beast = boost::beast;  // Boost.Beast 命名空间，用于 HTTP 处理
namespace http = beast::http;  // HTTP 相关操作
namespace net = boost::asio;  // Boost.Asio 命名空间，用于异步网络编程
using tcp = boost::asio::ip::tcp;  // TCP 相关操作

class CServer;  // 服务器类前置声明
class LogicSystem;  // 逻辑系统类前置声明

/**
 * @class CSession
 * @brief 代表一个 TCP 连接的会话，管理客户端的通信
 */
class CSession : public std::enable_shared_from_this<CSession>
{
public:

    CSession(boost::asio::io_context& io_context, CServer* server);

    ~CSession();

    tcp::socket& GetSocket();



    string& GetUuid();

    void Start();


    void Send(char* msg, short max_length, short msgid);


    void Send(std::string msg, short msgid);

    void Close();

 
    std::shared_ptr<CSession> SharedSelf();


    void AsyncReadBody(int length);

    void AsyncReadHead(int total_len);

private:

    void asyncReadFull(std::size_t maxLength,
        std::function<void(const boost::system::error_code&, std::size_t)> handler);

    /**
     * @brief 分块异步读取数据
     * @param read_len 当前读取长度
     * @param total_len 需要读取的总长度
     * @param handler 读取完成后的回调函数
     */
    void asyncReadLen(std::size_t read_len,
        std::size_t total_len,
        std::function<void(const boost::system::error_code&, std::size_t)> handler);

    /**
     * @brief 处理写入数据后的回调
     * @param error 发生的错误（如果有）
     * @param shared_self 指向当前会话的 `shared_ptr`
     */
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    tcp::socket _socket;  // 当前会话的 TCP socket
    //std::string _session_id;  // 当前会话的唯一 ID
    char _data[MAX_LENGTH];  // 读取数据的缓冲区
    CServer* _server;  // 服务器指针
    bool _b_close;  // 是否关闭会话
    std::queue<shared_ptr<SendNode>> _send_que;  // 发送消息队列
    std::mutex _send_lock;  // 互斥锁，确保发送队列的线程安全
    std::shared_ptr<RecvNode> _recv_msg_node;  // 当前接收的消息结构
    bool _b_head_parse;  // 是否解析了头部
    std::shared_ptr<MsgNode> _recv_head_node;  // 存储解析出的消息头
    string _uuid;  // 用户 ID
};

/**
 * @class LogicNode
 * @brief 逻辑处理节点，封装了一个会话和接收到的消息
 */
class LogicNode {
    friend class LogicSystem;  // 允许 LogicSystem 访问私有成员
public:
    /**
     * @brief 构造函数，初始化会话和接收到的消息
     * @param session 关联的会话对象
     * @param recvnode 关联的接收消息对象
     */
    LogicNode(shared_ptr<CSession> session, shared_ptr<RecvNode> recvnode);

private:
    shared_ptr<CSession> _session;  // 关联的会话
    shared_ptr<RecvNode> _recvnode;  // 关联的接收消息
};
