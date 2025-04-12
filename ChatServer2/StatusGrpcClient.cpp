#include "StatusGrpcClient.h"

// 向 StatusServer 发起 GetChatServer 请求，用于获取最优的 ChatServer 地址和连接信息
GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context; // gRPC 客户端上下文
	GetChatServerRsp reply; // 用于接收响应
	GetChatServerReq request; // 构造请求体
	request.set_uid(uid); // 设置用户 UID

	auto stub = pool_->getConnection(); // 从连接池获取一个 stub
	Status status = stub->GetChatServer(&context, request, &reply); // 发起 gRPC 请求

	// 使用 defer 自动归还 stub 到连接池
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});

	// 返回结果，若调用失败则标记错误码
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

// 向 StatusServer 发起 Login 请求，用于校验 uid 和 token 的合法性
LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context; // gRPC 客户端上下文
	LoginRsp reply; // 用于接收响应
	LoginReq request; // 构造请求体
	request.set_uid(uid); // 设置用户 ID
	request.set_token(token); // 设置用户 Token

	auto stub = pool_->getConnection(); // 获取 stub
	Status status = stub->Login(&context, request, &reply); // 发起登录请求

	// 自动归还 stub 到连接池
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});

	// 返回结果，如果失败则附加错误码
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

// 构造函数：初始化 StatusGrpcClient，并创建 gRPC 连接池
StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst(); // 读取配置
	std::string host = gCfgMgr["StatusServer"]["Host"]; // 获取 host
	std::string port = gCfgMgr["StatusServer"]["Port"]; // 获取 port

	// 创建连接池，默认大小为 5
	pool_.reset(new StatusConPool(5, host, port));
}
