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



// VerifyGrpcClient 类继承自 Singleton 模板类
// 用于与 VarifyService 服务进行 gRPC 通信
class VerifyGrpcClient : public Singleton<VerifyGrpcClient> {
    friend class Singleton<VerifyGrpcClient>;
public:
    // 获取验证码的方法
    GetVarifyRsp GetVarifyCode(const std::string& email) {
        // 创建 gRPC 客户端上下文
        ClientContext context;
        // 创建响应对象
        GetVarifyRsp reply;
        // 创建请求对象并设置电子邮件
        GetVarifyReq request;
        request.set_email(email);

        // 调用rpc生成的服务端方法，并获取状态和响应
        Status status = stub_->GetVarifyCode(&context, request, &reply);

        if (status.ok()) {
            
            return reply;
        }
        else {
            std::cout << status.error_code() << std::endl;
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
    }

private:
    VerifyGrpcClient() {
        // 创建一个指向服务端的通道，信使通信需要用到这个通道
        std::shared_ptr<Channel> channel = grpc::CreateChannel(
            "127.0.0.1:50051", // 服务端地址
            grpc::InsecureChannelCredentials() // 这里使用不安全的通道凭据
        );
        // 创建服务端存根
        stub_ = VarifyService::NewStub(channel);
    }

    // 存根对象，也可以成为信使，用于调用服务端的方法
    std::unique_ptr<VarifyService::Stub> stub_;
};
