#pragma once

#include <string>
#include "const.h"
#include <iostream>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class LogicSystem;

// 消息节点基础类，表示一段消息数据
class MsgNode {
public:
    // 构造函数：分配指定长度的内存，并初始化为 0
    MsgNode(short max_len) : _total_len(max_len), _cur_len(0) {
        _data = new char[_total_len + 1](); // 多分配一个字节用于字符串结尾 '\0'
        _data[_total_len] = '\0'; // 确保以 null 结尾，方便调试打印
    }

    // 析构函数：释放动态分配的内存
    ~MsgNode() {
        // std::cout << "destruct MsgNode" << endl; // 可用于调试析构是否被调用
        delete[] _data;
    }

    // 清空数据内容，重置当前长度
    void Clear() {
        ::memset(_data, 0, _total_len);
        _cur_len = 0;
    }

    short _cur_len;     // 当前已经写入的数据长度
    short _total_len;   // 分配的总长度
    char* _data;        // 指向数据内容的指针
};

// 接收消息节点，继承自 MsgNode，额外带有消息 ID
class RecvNode : public MsgNode {
    friend class LogicSystem; // 允许 LogicSystem 访问其私有成员
public:
    // 构造函数：初始化接收数据长度和对应的消息 ID
    RecvNode(short max_len, short msg_id);

public:
    short _msg_id; // 消息 ID，用于标识消息类型
};

// 发送消息节点，继承自 MsgNode，带有构造初始化内容的逻辑
class SendNode : public MsgNode {
    friend class LogicSystem;
public:
    // 构造函数：将要发送的数据内容和消息 ID 组装为一个完整数据包
    SendNode(const char* msg, short max_len, short msg_id);

private:
    short _msg_id; // 消息 ID，用于封包头
};
