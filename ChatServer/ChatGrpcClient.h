#pragma once

#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

// 引入 gRPC 所需命名空间
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

// 引入 proto 定义的消息类型
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

using message::FileMetaReq;
using message::FileMetaRsp;
using message::FileChunkReq;
using message::FileChunkRsp;
using message::FileFinishReq;
using message::FileFinishRsp;


/*
 * ChatConPool 是一个 gRPC 客户端连接池类，负责管理一组 ChatService 的 Stub 连接
 * 实现连接复用，减少创建连接的性能开销，支持多线程安全访问
 */
class ChatConPool {
public:
    // 构造函数：初始化连接池，创建多个 gRPC stub
    ChatConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            // 创建 gRPC channel 和 stub，使用非加密通信
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
                grpc::InsecureChannelCredentials());
            connections_.push(ChatService::NewStub(channel));
        }
    }

    // 析构函数：清理连接池并关闭
    ~ChatConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close();  // 标记连接池已关闭
        while (!connections_.empty()) {
            connections_.pop();  // 清空连接队列
        }
    }

    // 获取一个可用连接（阻塞等待），用于发起 gRPC 请求
    std::unique_ptr<ChatService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            return b_stop_ || !connections_.empty();  // 等待有可用连接或池被关闭
            });

        if (b_stop_) {
            return nullptr;
        }

        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }

    // 将连接归还回连接池
    void returnConnection(std::unique_ptr<ChatService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) return;
        connections_.push(std::move(context));
        cond_.notify_one();  // 通知等待线程
    }

    // 关闭连接池，唤醒所有等待线程
    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;   // 标志是否关闭连接池
    size_t poolSize_;            // 连接池大小
    std::string host_;           // 服务器地址
    std::string port_;           // 服务器端口
    std::queue<std::unique_ptr<ChatService::Stub>> connections_; // gRPC 连接队列
    std::mutex mutex_;
    std::condition_variable cond_;
};

/*
 * ChatGrpcClient 是一个 gRPC 客户端封装类（单例），用于和多个 ChatServer 进行通信
 * 支持：添加好友、验证好友、聊天消息转发、获取用户信息等功能
 * 内部维护多个 ChatConPool，分别对应不同的服务器
 */
class ChatGrpcClient : public Singleton<ChatGrpcClient> {
    friend class Singleton<ChatGrpcClient>;
public:
    ~ChatGrpcClient() {}

    // 好友请求通知到目标服务器
    AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);

    // 通知目标服务器好友申请已验证
    AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);

    // 获取指定用户的基础信息（先查 redis，再查 mysql）
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

	// 聊天消息转发到目标服务器
    TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);


    FileMetaRsp   NotifyFileMeta(std::string server_ip,
        const FileMetaReq& req);
    FileChunkRsp  NotifyFileChunk(std::string server_ip,
        const FileChunkReq& req);
    FileFinishRsp NotifyFileFinish(std::string server_ip,
        const FileFinishReq& req);
private:
    // 构造函数私有化，由 Singleton 管理对象唯一性
    ChatGrpcClient();

    // 保存所有服务器的连接池，key 为 server_name，value 为对应连接池
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};
