#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <climits>

// 生成一个全局唯一的字符串标识符，用作登录 token
std::string generate_unique_string() {
	boost::uuids::uuid uuid = boost::uuids::random_generator()(); // 生成 UUID
	std::string unique_string = to_string(uuid); // 转为字符串
	return unique_string;
}

// 处理客户端获取聊天服务器请求，返回当前连接最少的服务器信息并生成登录 token
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{

	std::string prefix("wjf status server has received :  ");
	// 选择连接数最少的服务器
	const auto& server = getChatServer();

	// 构建响应
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);

	// 为当前用户生成唯一 token，并存入 redis
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());

	return Status::OK;
}

// 构造函数，读取配置文件中所有聊天服务器的信息，并存入本地 map
StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst(); // 获取配置管理器
	auto server_list = cfg["chatservers"]["Name"]; // 获取服务器列表（逗号分隔）

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	// 将服务器名按逗号分隔存入 words 向量
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	// 遍历每个服务器配置，提取 name/host/port 并存入 _servers 成员变量
	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}
}

// 获取当前连接数最少的聊天服务器
ChatServer StatusServiceImpl::getChatServer() {

	std::lock_guard<std::mutex> guard(_server_mtx); // 加锁保证线程安全

	// 初始最小服务器设为第一个（将连接数设为最大值）
	auto minServer = _servers.begin()->second;

	// 尝试从 Redis 中读取该服务器的当前连接数
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}

	// 遍历所有服务器，找出连接数最少的
	for (auto& server : _servers) {
		if (server.second.name == minServer.name) {
			continue;
		}

		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}
	return minServer;
}

// 处理客户端登录请求，验证 token 是否有效
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	std::cout << "Login request received from user: " << request->uid() << std::endl;

	auto uid = request->uid();
	auto token = request->token();
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string online_key = "user_online_status_" + uid_str;

	std::string token_value;
	std::string online_value;

	// 检查 token 是否匹配
	if (!RedisMgr::GetInstance()->Get(token_key, token_value)) {
		std::cout << "Token not found for uid: " << uid << std::endl;
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	if (token_value != token) {
		std::cout << "Token mismatch for uid: " << uid << std::endl;
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	// 检查是否已经登录（在线）
	bool online_exists = RedisMgr::GetInstance()->Get(online_key, online_value);
	if (online_exists && online_value == "1") {
		std::cout << "User " << uid << " already logged in elsewhere." << std::endl;
		reply->set_error(ErrorCodes::UidInvalid); // 或定义新的重复登录错误码
		return Status::OK;
	}

	// 设置为在线状态
	RedisMgr::GetInstance()->Set(online_key, "1");

	std::cout << "User " << uid << " logged in successfully." << std::endl;
	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}










// 将 uid 对应的 token 写入 Redis，供后续登录验证
void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}
