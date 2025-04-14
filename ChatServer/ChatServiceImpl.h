#pragma once

// gRPC 相关头文件
#include <grpcpp/grpcpp.h>

// Proto 文件生成的服务定义和消息结构
#include "message.grpc.pb.h"
#include "message.pb.h"

// 线程安全相关
#include <mutex>

// 用户数据结构
#include "data.h"

// 使用 gRPC 中的常用类
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// 使用 protobuf 定义的消息结构
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService; // 服务接口基类
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

// ChatServiceImpl 是 ChatServer 上真正处理业务逻辑的 gRPC 服务实现类
class ChatServiceImpl final : public ChatService::Service
{
public:
    // 构造函数，可以在其中初始化所需资源（比如日志、数据库等）
    ChatServiceImpl();

    // 好友申请通知到目标服务器
    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;

	// 通知目标服务器好友申请已验证
    Status NotifyAuthFriend(ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;

    // 文本聊天消息通知的 RPC 实现
    Status NotifyTextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;

    // 获取用户基本信息（优先查 Redis，未命中则查 MySQL）
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
};
