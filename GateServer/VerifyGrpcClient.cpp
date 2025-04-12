#include "VerifyGrpcClient.h"

#include "ConfigMgr.h"


// VerifyGrpcClient 的构造函数 内容是从配置文件中读取服务器的地址和端口号
VerifyGrpcClient::VerifyGrpcClient() {
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VarifyServer"]["Host"]; // 读取配置文件中的服务器地址
	std::string port = gCfgMgr["VarifyServer"]["Port"]; // 读取配置文件中的服务器端口号
	pool_.reset(new RPConPool(5, host, port));
	//pool_ = std::make_unique<RPConPool>(5, host, port); 
	//也可以用make_unique,用法是make_unique<RPConPool>(5, host, port)
	// 创建一个连接池,reset是智能指针的方法,用于释放原有的指针并重新指向新的指针
	// 这里使用reset是因为pool_是一个unique_ptr,不能直接赋值
}



GetVarifyRsp VerifyGrpcClient::GetVarifyCode(const std::string& email)
{
	// 创建 gRPC 客户端上下文
	ClientContext context;
	// 创建响应对象
	GetVarifyRsp reply;
	// 创建请求对象并设置电子邮件
	GetVarifyReq request;
	request.set_email(email);

	// 调用rpc生成的服务端方法，并获取状态和响应
	auto stub = pool_->getConnection();
	Status status = stub->GetVarifyCode(&context, request, &reply);

	if (status.ok()) {
		pool_->returnConnection(std::move(stub));
		return reply;
	}
	else {
		pool_->returnConnection(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

