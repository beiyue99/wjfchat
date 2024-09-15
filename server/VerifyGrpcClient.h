#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"  // �������ɵ� gRPC ͷ�ļ�
#include "const.h"           // ���볣�������ͷ�ļ�
#include "Singleton.h"      // ���뵥��ģʽ��ͷ�ļ�

// ʹ�� gRPC �����ռ��е���
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

// ʹ�� message �����ռ��е���Ϣ�ͷ�����
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;



// VerifyGrpcClient ��̳��� Singleton ģ����
// ������ VarifyService ������� gRPC ͨ��
class VerifyGrpcClient : public Singleton<VerifyGrpcClient> {
    friend class Singleton<VerifyGrpcClient>;
public:
    // ��ȡ��֤��ķ���
    GetVarifyRsp GetVarifyCode(const std::string& email) {
        // ���� gRPC �ͻ���������
        ClientContext context;
        // ������Ӧ����
        GetVarifyRsp reply;
        // ��������������õ����ʼ�
        GetVarifyReq request;
        request.set_email(email);

        // ����rpc���ɵķ���˷���������ȡ״̬����Ӧ
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
        // ����һ��ָ�����˵�ͨ������ʹͨ����Ҫ�õ����ͨ��
        std::shared_ptr<Channel> channel = grpc::CreateChannel(
            "127.0.0.1:50051", // ����˵�ַ
            grpc::InsecureChannelCredentials() // ����ʹ�ò���ȫ��ͨ��ƾ��
        );
        // ��������˴��
        stub_ = VarifyService::NewStub(channel);
    }

    // �������Ҳ���Գ�Ϊ��ʹ�����ڵ��÷���˵ķ���
    std::unique_ptr<VarifyService::Stub> stub_;
};
