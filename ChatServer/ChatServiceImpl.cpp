#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "RedisMgr.h"
#include "MysqlMgr.h"


// 构造函数（不做额外处理）
ChatServiceImpl::ChatServiceImpl()
{

}







grpc::Status ChatServiceImpl::NotifyFileMeta(
	grpc::ServerContext*,
	const FileMetaReq* req,
	FileMetaRsp* rsp)
{
	try {
		string meta_key = "file_meta_" + req->file_id();
		RedisMgr::GetInstance()->HSet(meta_key, "size",
			std::to_string(req->file_size()));
		RedisMgr::GetInstance()->HSet(meta_key, "fname", req->file_name());
		RedisMgr::GetInstance()->HSet(meta_key, "from",
			std::to_string(req->fromuid()));
		RedisMgr::GetInstance()->HSet(meta_key, "to",
			std::to_string(req->touid()));

		RedisMgr::GetInstance()->Set("file_recv_" + req->file_id(), "0");

		FileTransferMgr::Inst().createOrGet(
			req->file_id(), "./store/" + req->file_id() + ".tmp",
			req->file_size());

		rsp->set_error(Success);
		rsp->set_file_id(req->file_id());
		rsp->set_recv_size(0);
		return grpc::Status::OK;
	}
	catch (std::exception& e) {
		rsp->set_error(RPCFailed);
		return grpc::Status::CANCELLED;
	}
}

grpc::Status ChatServiceImpl::NotifyFileChunk(
	grpc::ServerContext*,
	const FileChunkReq* req,
	FileChunkRsp* rsp)
{
	try {
		int64_t new_off = FileTransferMgr::Inst().append(
			req->file_id(), req->offset(),
			req->data().data(), req->data().size());

		RedisMgr::GetInstance()->Set(
			"file_recv_" + req->file_id(), std::to_string(new_off));

		rsp->set_error(Success);
		rsp->set_file_id(req->file_id());
		rsp->set_offset(new_off);
		return grpc::Status::OK;
	}
	catch (std::exception& e) {
		rsp->set_error(RPCFailed);
		return grpc::Status::CANCELLED;
	}
}

grpc::Status ChatServiceImpl::NotifyFileFinish(
	grpc::ServerContext*,
	const FileFinishReq* req,
	FileFinishRsp* rsp)
{
	try {
		FileTransferMgr::Inst().finish(req->file_id());
		RedisMgr::GetInstance()->Del("file_meta_" + req->file_id());
		RedisMgr::GetInstance()->Del("file_recv_" + req->file_id());

		rsp->set_error(Success);
		rsp->set_file_id(req->file_id());
		return grpc::Status::OK;
	}
	catch (std::exception& e) {
		rsp->set_error(RPCFailed);
		return grpc::Status::CANCELLED;
	}
}



// 好友申请通知到目标服务器
// 会通过 GRPC 从其它 ChatServer 发过来
Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
	auto touid = request->touid();  // 获取目标用户 UID
	auto session = UserMgr::GetInstance()->GetSession(touid);  // 查询目标用户是否在线（有会话）

	// 自动设置响应值（即使提前 return 也能正确设置）
	Defer defer([request, reply]() {
		reply->set_error(ErrorCodes::Success);
		reply->set_applyuid(request->applyuid());
		reply->set_touid(request->touid());
		});

	// 用户不在线（不在该服务器内存中），直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	// 构造 JSON 数据通知目标用户
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["applyuid"] = request->applyuid();
	rtvalue["name"] = request->name();
	rtvalue["icon"] = request->icon();

	std::string return_str = rtvalue.toStyledString(); // 将 JSON 转为字符串
	session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);  // 通过 Session 发送到客户端
	return Status::OK;
}

// 通知目标服务器好友申请已验证
Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* reply)
{
	auto touid = request->touid();    // 被通知的人
	auto fromuid = request->fromuid();  // 发出同意的人
	auto session = UserMgr::GetInstance()->GetSession(touid);  

	Defer defer([request, reply]() {
		reply->set_error(ErrorCodes::Success);
		reply->set_fromuid(request->fromuid());
		reply->set_touid(request->touid());
		});

	// 不在线则不发送通知
	if (session == nullptr) {
		return Status::OK;
	}

	// 构造 JSON 数据
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	// 获取对方的用户信息（用于头像、昵称等展示）
	std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
	auto user_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, fromuid, user_info);
	if (b_info) {
		rtvalue["uid"] = user_info->uid;
		rtvalue["pwd"] = user_info->pwd;
		rtvalue["email"] = user_info->email;
		rtvalue["name"] = user_info->name;
		rtvalue["icon"] = user_info->icon;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}

	std::string return_str = rtvalue.toStyledString();
	session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);  // 发送到客户端
	return Status::OK;
}

// 处理文本聊天消息通知
Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
	const TextChatMsgReq* request, TextChatMsgRsp* reply)
{
	auto touid = request->touid();  // 接收方 UID
	auto session = UserMgr::GetInstance()->GetSession(touid);  // 检查是否在线

	reply->set_error(ErrorCodes::Success);

	// 不在线则直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	// 构造 JSON 数据
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	// 将消息数组打包成 JSON
	Json::Value text_array;
	for (auto& msg : request->textmsgs()) {
		Json::Value element;
		element["content"] = msg.msgcontent();
		element["msgid"] = msg.msgid();
		text_array.append(element);
	}
	rtvalue["text_array"] = text_array;

	std::string return_str = rtvalue.toStyledString();
	session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);  // 发给客户端
	return Status::OK;
}

// 获取用户基础信息（优先从 Redis 获取，Redis 没有则从 MySQL 获取并写入 Redis）
bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);  // 查 Redis

	if (b_base) {
		// 从 Redis 中解析 JSON
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);

		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->icon = root["icon"].asString();

		//std::cout << "user login uid is  " << userinfo->uid << " name  is "
		//	<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
		return true;
	}
	else {
		// Redis 中没找到，查 MySQL
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
		return true;
	}
}
