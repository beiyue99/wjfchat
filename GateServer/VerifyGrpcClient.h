#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"  // 引入生成的 gRPC 头文件
#include "const.h"           // 引入常量定义的头文件
#include "Singleton.h"      // 引入单例模式的头文件

// 使用 gRPC 命名空间中的类
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

// 使用 message 命名空间中的消息和服务定义
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;




class RPConPool {
public:
    RPConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials()); //InsecureChannelCredentials代表不使用加密连接
			connections_.push(VarifyService::NewStub(channel)); 
        }
    }
    ~RPConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close();
        while (!connections_.empty()) {
            connections_.pop();
        }
    }
	// stub的作用是将请求发送到服务器端
    std::unique_ptr<VarifyService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
				return true;   //如果停止则直接返回true,继续执行
            }
			return !connections_.empty(); //如果不为空则返回true,继续执行,否则等待
            });
        //如果停止则直接返回空指针
        if (b_stop_) {
            return  nullptr;
        }
		auto context = std::move(connections_.front()); //move作用是将connections_.front()的值转移到context中
        connections_.pop();
        return context;
    }
    
	// 归还连接到连接池
    void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(std::move(context));
        cond_.notify_one();
    }
    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }
private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VarifyService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};



// VerifyGrpcClient 类继承自 Singleton 模板类
// 用于与 VarifyService 服务进行 gRPC 通信
class VerifyGrpcClient : public Singleton<VerifyGrpcClient> {
    friend class Singleton<VerifyGrpcClient>;
public:
    // 获取验证码的方法
    GetVarifyRsp GetVarifyCode(const std::string& email);

private:
    VerifyGrpcClient(); 
	std::unique_ptr<RPConPool> pool_; // 连接池
};
