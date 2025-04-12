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

// 处理添加好友请求，通知目标用户（如果在线）
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
	rtvalue["desc"] = request->desc();
	rtvalue["icon"] = request->icon();
	rtvalue["sex"] = request->sex();
	rtvalue["nick"] = request->nick();

	std::string return_str = rtvalue.toStyledString(); // 将 JSON 转为字符串
	session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);  // 通过 Session 发送到客户端
	return Status::OK;
}

// 处理添加好友被同意的通知
Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* reply)
{
	auto touid = request->touid();    // 被通知的人
	auto fromuid = request->fromuid();  // 发出同意的人
	auto session = UserMgr::GetInstance()->GetSession(touid);  // 查询被通知人是否在线

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
		rtvalue["desc"] = user_info->desc;
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
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
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
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
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;

		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
		return true;
	}
}
