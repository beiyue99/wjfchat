#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"
#include "CSession.h"
#include "MysqlMgr.h"

// 构造函数，初始化所有目标聊天服务器的连接池（gRPC Stub 对象）
ChatGrpcClient::ChatGrpcClient()
{
	auto& cfg = ConfigMgr::Inst(); // 获取配置管理器
	auto server_list = cfg["PeerServer"]["Servers"]; // 从配置中读取服务列表（逗号分隔）

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	// 按 , 分割服务器名
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	// 遍历所有服务器名称，构建连接池
	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
	}
}

// 通知目标服务器发起添加好友请求
AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;

	// 用 Defer 设置默认返回值，确保返回结构体带有必要字段
	Defer defer([&rsp, &req]() {
		rsp.set_error(ErrorCodes::Success);
		rsp.set_applyuid(req.applyuid());
		rsp.set_touid(req.touid());
		});

	// 查找对应服务器的连接池
	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp; // 未找到该服务器连接池
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection(); // 获取 gRPC 连接

	// 发起远程调用
	Status status = stub->NotifyAddFriend(&context, req, &rsp);

	// 调用完成后归还连接
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	// 若调用失败，修改错误码
	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}

// 优先从 Redis 获取用户信息，如果没有就从 MySQL 获取并缓存到 Redis
bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	std::string info_str = "";

	// 从 Redis 获取
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->icon = root["icon"].asString();

		// 打印调试信息
		//std::cout << "user login uid is  " << userinfo->uid << " name  is "
		//	<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email 
		//	<< std::endl << "icon is " << userinfo->icon << std::endl;
	}
	else {
		// Redis 缓存未命中，则访问 MySQL
		std::shared_ptr<UserInfo> user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		// 写入 Redis 缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["icon"] = userinfo->icon;

		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}

// 通知目标服务器发起好友认证请求（同意添加好友）
AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		});

	// 获取对应服务器连接池
	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();

	// 发送 gRPC 请求
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);

	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}
	return rsp;
}

// 通知目标服务器转发文本聊天消息
TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
	const TextChatMsgReq& req, const Json::Value& rtvalue)
{
	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());

		// 将请求中的每条消息写入响应
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}
		});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();

	// 发送 gRPC 请求
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);

	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}
