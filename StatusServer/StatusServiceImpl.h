#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;



struct ChatServer {
	std::string host;
	std::string port;
	std::string name;
	int con_count;
};



class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl(); 
	// 获取当前连接数最少的聊天服务器
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	// 处理客户端登录请求，验证 token 是否有效
	Status Login(ServerContext* context, const LoginReq* request,LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);  // 将 uid 对应的 token 写入 Redis
	ChatServer getChatServer();
	std::unordered_map<std::string, ChatServer> _servers; // 服务器列表, key为服务器名字, value为服务器
	std::mutex _server_mtx;
};

