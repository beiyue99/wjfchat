#pragma once

#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <queue>
#include <condition_variable>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::StatusService;

/*
 * StatusConPool 类：用于管理与 StatusServer 的 gRPC 连接池
 */
class StatusConPool {
public:
    // 构造函数，初始化连接池（指定大小 poolSize），连接到目标 host + port 的 StatusService
    StatusConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
                grpc::InsecureChannelCredentials()); // 创建明文连接

            // 将新建的 stub 放入连接池队列
            connections_.push(StatusService::NewStub(channel));
        }
    }

    // 析构函数：关闭连接池、清空所有 stub
    ~StatusConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close();
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    // 从连接池中获取一个可用的 stub 连接
    std::unique_ptr<StatusService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });

        if (b_stop_) {
            return nullptr;
        }

        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }

    // 使用完连接后归还到连接池
    void returnConnection(std::unique_ptr<StatusService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(std::move(context));
        cond_.notify_one(); // 通知等待线程有新连接可以用
    }

    // 关闭连接池
    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_; // 是否停止连接池
    size_t poolSize_;          // 连接池大小
    std::string host_;         // 目标主机 IP
    std::string port_;         // 目标端口
    std::queue<std::unique_ptr<StatusService::Stub>> connections_; // 存储连接的队列
    std::mutex mutex_;
    std::condition_variable cond_;
};

/*
 * StatusGrpcClient：gRPC 客户端封装类，提供对 StatusServer 的远程调用接口
 * 采用单例模式，统一管理连接池的生命周期
 */
class StatusGrpcClient : public Singleton<StatusGrpcClient> {
    friend class Singleton<StatusGrpcClient>;

public:
    ~StatusGrpcClient() {}

    // 调用 StatusServer 的 GetChatServer 接口，根据 uid 返回最合适的聊天服务器
    GetChatServerRsp GetChatServer(int uid);

    // 登录时调用，校验 token 是否有效
    LoginRsp Login(int uid, std::string token);

private:
    StatusGrpcClient(); // 构造函数内部完成连接池初始化
    std::unique_ptr<StatusConPool> pool_; // 连接池指针
};

