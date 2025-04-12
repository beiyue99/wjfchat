#include "MsgNode.h"

// RecvNode 构造函数
// 参数 max_len：消息内容的最大长度
// 参数 msg_id：消息的 ID（标识消息类型）
// 它本质是从 MsgNode 继承来的结构，用于封装接收到的数据
RecvNode::RecvNode(short max_len, short msg_id)
    : MsgNode(max_len), // 调用基类 MsgNode 构造函数，初始化缓冲区长度
    _msg_id(msg_id)   // 设置消息 ID
{
    // 无其他逻辑，RecvNode 只是简单绑定消息 ID 和长度
}

// SendNode 构造函数
// 参数 msg：待发送的原始消息数据（不含包头）
// 参数 max_len：消息数据的长度（不含包头）
// 参数 msg_id：消息的 ID（将会写入包头中）
SendNode::SendNode(const char* msg, short max_len, short msg_id)
// 调用基类 MsgNode 构造函数，分配总长度 = 包头长度 + 实际消息长度
    : MsgNode(max_len + HEAD_TOTAL_LEN),
    _msg_id(msg_id)
{
    // 将 msg_id（消息类型）转换为网络字节序，并拷贝到 _data 的前两个字节
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &msg_id_host, HEAD_ID_LEN); // 写入头部的消息 ID 字段

    // 将消息长度 max_len 转换为网络字节序，并拷贝到 _data 的接下来两个字节
    short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN); // 写入头部的数据长度字段

    // 将消息主体内容 msg 拷贝到 _data 的第 4 个字节起的位置
    memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
    // 最终 _data 的结构是：
    // | 2字节msg_id | 2字节长度 | 消息内容... |
}
