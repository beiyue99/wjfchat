#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "FileTransferMgr.h"
#include "ChatGrpcClient.h"
#include <boost/beast/core/detail/base64.hpp>
#include "CServer.h"        // ★ 必须加
using namespace std;
namespace b64 = boost::beast::detail::base64;

LogicSystem::LogicSystem():_b_stop(false){
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}


/* ---------- Base64 工具 ---------- */
static std::string decode64(const std::string& in)
{
	std::string out;
	out.resize(boost::beast::detail::base64::decoded_size(in.size()));

	// 这行改动：把 out.data() 换成 &out[0]
	auto sz = boost::beast::detail::base64::decode(&out[0],
		in.data(),
		in.size());

	out.resize(sz.first);     // sz.first = 实际写入字节数
	return out;
}


/* ---------- 发送文件元数据 ---------- */
void LogicSystem::FileMeta(
	shared_ptr<CSession> sess,
	const short&, const string& dataRaw)
{
	/* 1. 解析客户端发来的 JSON ---------------------------- */
	Json::Value root;  Json::Reader rd;  rd.parse(dataRaw, root);

	/* 2. 把 fromuid 改成真正的登录 uid -------------------- */
	int32_t  fromuid = sess->GetUserId();   // ★ 核心
	root["fromuid"] = fromuid;             // 覆盖
	std::cout << "from id is " << fromuid << std::endl;
	/* 3. 其它字段照旧 ------------------------------------ */
	string   file_id = root["file_id"].asString();
	int64_t  fsize = root["size"].asInt64();
	int32_t  touid = root["touid"].asInt();
	string   fname = root["fname"].asString();

	/* 4. 用 root 重新序列化，后面都用这一份 --------------- */
	string data = root.toStyledString();

	/* ……下面原有的 Redis / FileTransferMgr / ACK 逻辑保持不变 …… */

	/* 存 Redis */
	string meta_key = "file_meta_" + file_id;
	RedisMgr::GetInstance()->HSet(meta_key, "size", std::to_string(fsize));
	RedisMgr::GetInstance()->HSet(meta_key, "fname", fname);
	RedisMgr::GetInstance()->HSet(meta_key, "from", std::to_string(fromuid));
	RedisMgr::GetInstance()->HSet(meta_key, "to", std::to_string(touid));
	RedisMgr::GetInstance()->Set("file_recv_" + file_id, "0");

	/* 转发给接收方（同服在线的话） */
	if (auto to_sess = UserMgr::GetInstance()->GetSession(touid))
		to_sess->Send(data, ID_FILE_META_REQ);

	/* 回 ACK 给发送方 */
	Json::Value rsp;
	rsp["error"] = Success;
	rsp["file_id"] = file_id;
	rsp["recv_size"] = 0;
	sess->Send(rsp.toStyledString(), ID_FILE_META_RSP);

	std::cout << "[FORWARD] 1031 to uid=" << touid
		<< " len=" << data.size() << '\n';
}


/* ---------- 追加文件分片 ---------- */
void LogicSystem::FileChunk(
	shared_ptr<CSession> sess,
	const short&, const string& data)
{
	try {
		// 现有 JSON 解析、写盘代码
		Json::Value root;  Json::Reader rd;  rd.parse(data, root);
		string file_id = root["file_id"].asString();
		int64_t offset = root["offset"].asInt64();
		string bin_b64 = root["data"].asString();
		string bin = decode64(bin_b64);
		int32_t touid = root["touid"].asInt();

		int64_t new_off = FileTransferMgr::Inst().append(
			file_id, offset, bin.data(), bin.size());

		RedisMgr::GetInstance()->Set(
			"file_recv_" + file_id, std::to_string(new_off));

		// 转发给接收方（同服）
		auto to_sess = UserMgr::GetInstance()->GetSession(touid);
		if (to_sess)
			to_sess->Send(data, ID_FILE_DATA_REQ);

		// ACK
		Json::Value ack;
		ack["error"] = Success;
		ack["file_id"] = file_id;
		ack["offset"] = new_off;
		sess->Send(ack.toStyledString(), ID_FILE_DATA_RSP);

		//sess->ResetHeartbeat();
	}
	catch (const std::exception& e) {
		std::cerr << "FileChunk except: " << e.what() << std::endl;
		return;
	}
	
}

/* ---------- 文件发送完毕 ---------- */
void LogicSystem::FileFinish(
	shared_ptr<CSession> sess,
	const short&, const string& data)
{
	Json::Value root;  Json::Reader rd;  rd.parse(data, root);
	string file_id = root["file_id"].asString();
	int32_t touid = root["touid"].asInt();

	FileTransferMgr::Inst().finish(file_id);
	RedisMgr::GetInstance()->Del("file_meta_" + file_id);
	RedisMgr::GetInstance()->Del("file_recv_" + file_id);

	// 通知接收方
	auto to_sess = UserMgr::GetInstance()->GetSession(touid);
	if (to_sess)
		to_sess->Send(data, ID_FILE_FINISH_REQ);

	Json::Value rsp;
	rsp["error"] = Success;  rsp["file_id"] = file_id;
	sess->Send(rsp.toStyledString(), ID_FILE_FINISH_RSP);

	//sess->ResetHeartbeat();
}

/* ---------- 查询断点 ---------- */
void LogicSystem::FileResume(
	shared_ptr<CSession> sess,
	const short&, const string& data)
{
	Json::Value root;  Json::Reader rd;  rd.parse(data, root);
	string file_id = root["file_id"].asString();

	string cur = RedisMgr::GetInstance()->HGet(
		"file_recv_" + file_id, "").empty() ?
		"0" : RedisMgr::GetInstance()->HGet(
			"file_recv_" + file_id, "");

	Json::Value rsp;
	rsp["error"] = Success; rsp["file_id"] = file_id;
	rsp["recv_size"] = std::stoll(cur);
	sess->Send(rsp.toStyledString(), ID_FILE_RESUME_RSP);
}




void LogicSystem::PostMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	//由0变为1则发送通知信号
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}


void LogicSystem::FileAccept(std::shared_ptr<CSession> sess,
	const short&, const std::string& data)
{
	Json::Value root;  Json::Reader().parse(data, root);
	std::string fid = root["file_id"].asString();
	int         action = root["action"].asInt();  // 1 / 0

	/* 根据 file_meta 找到发送者 */
	std::string fromUidStr =
		RedisMgr::GetInstance()->HGet("file_meta_" + fid, "from");
	int fromUid = std::stoi(fromUidStr);

	auto toSess = UserMgr::GetInstance()->GetSession(fromUid);
	if (toSess) toSess->Send(data, ID_FILE_ACCEPT_RSP);
}


void LogicSystem::ForwardFileMetaRsp(std::shared_ptr<CSession> sess,
	const short& /*msgId*/,
	const std::string& body)
{
	// 1. 解析出 original sender / receiver
	Json::Value root;  Json::Reader rd;  rd.parse(body, root);
	int fromUid = root["fromuid"].asInt();     //  文件发送者
	int toUid = root["touid"].asInt();       //  文件接收者

	/* 2. 正向只做“透传” —— 发给对方那一端 */
	int targetUid = (sess->GetUserId() == fromUid) ? toUid : fromUid;
	auto peerSess = UserMgr::GetInstance()->GetSession(targetUid);
	if (peerSess)
		peerSess->Send(body, ID_FILE_META_RSP);   // 1032 原封不动丢过去
}



/* ---------- 1032  FileMetaAck ---------- */
void LogicSystem::FileMetaAck(std::shared_ptr<CSession> sess,
	const short&, const std::string& data)
{
	Json::Value root;  Json::Reader rd;  rd.parse(data, root);
	int32_t touid = root["fromuid"].asInt();   // 发回原发送者

	if (auto to = UserMgr::GetInstance()->GetSession(touid))
		to->Send(data, ID_FILE_META_RSP);

	/* 这里不需要再回给接收端，可忽略 */
	(void)sess;
}



/* ---------- 接收方确认 1032 ---------- */
void LogicSystem::FileMetaRsp(shared_ptr<CSession> sess,
	const short&, const string& data)
{
	Json::Value root; Json::Reader rd; rd.parse(data, root);
	string file_id = root["file_id"].asString();
	int32_t fromuid = root["fromuid"].asInt();   // 发送者 uid
	int32_t touid = root["touid"].asInt();     // =自己
	int accepted = root["accepted"].asInt();  // 1/0

	if (accepted != 1) return;                   // 拒绝直接丢弃即可

	auto send_sess = UserMgr::GetInstance()->GetSession(fromuid);
	if (send_sess)  send_sess->Send(data, ID_FILE_META_RSP);
}



void LogicSystem::DealMsg() {
	// 无限循环处理逻辑消息队列中的消息
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex); // 加锁，保护消息队列的并发访问

		// 如果消息队列为空且未收到停服信号，则阻塞等待消息到来
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk); // 等待其他线程投递消息并唤醒
		}

		// 如果收到停服信号（_b_stop 为 true），则把剩余消息处理完再退出
		if (_b_stop) {
			while (!_msg_que.empty()) {
				// 取出队头消息节点
				auto msg_node = _msg_que.front();
				cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;

				// 根据消息 ID 查找对应的处理回调函数
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					// 如果未注册对应回调函数，则忽略消息
					_msg_que.pop();
					continue;
				}

				// 执行注册的回调函数，处理消息
				call_back_iter->second(
					msg_node->_session,                      // 会话对象
					msg_node->_recvnode->_msg_id,           // 消息ID
					std::string(msg_node->_recvnode->_data, // 消息内容
						msg_node->_recvnode->_cur_len)
				);

				_msg_que.pop(); // 消息处理完后从队列中移除
			}
			break; // 跳出 for 循环，线程退出
		}

		// 正常情况下处理消息队列（未停服）
		auto msg_node = _msg_que.front(); // 取出消息
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;

		// 查找是否注册了对应的消息回调处理函数
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			// 如果未注册该消息类型的处理器，忽略并继续下一个
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}

		// 找到了处理函数，执行处理
		call_back_iter->second(
			msg_node->_session,                       // 会话对象
			msg_node->_recvnode->_msg_id,            // 消息ID
			std::string(msg_node->_recvnode->_data,  // 消息内容
				msg_node->_recvnode->_cur_len)
		);

		_msg_que.pop(); // 从队列中移除已处理的消息
	}
}



// 注册各类消息对应的处理函数（回调函数）
void LogicSystem::RegisterCallBacks() {

	// 注册处理登录消息的回调函数
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	// bind 用于将成员函数和 this 绑定，并保留参数 _1, _2, _3 占位符，代表
	// (std::shared_ptr<CSession>, short msgid, std::string data)

	// 注册处理搜索用户请求的回调函数
	_fun_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	// 注册处理添加好友申请请求的回调函数
	_fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	// 注册处理好友认证（同意/拒绝添加好友）的请求处理函数
	_fun_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	// 注册处理文本消息发送请求的处理函数（即聊天消息）
	_fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_HEARTBEAT_REQ] = std::bind(&LogicSystem::Heartbeat,
		this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); // ★ 新增


	_fun_callbacks[ID_FILE_META_REQ] =
		std::bind(&LogicSystem::FileMeta, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

	_fun_callbacks[ID_FILE_DATA_REQ] =
		std::bind(&LogicSystem::FileChunk, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

	_fun_callbacks[ID_FILE_FINISH_REQ] =
		std::bind(&LogicSystem::FileFinish, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

	_fun_callbacks[ID_FILE_RESUME_REQ] =
		std::bind(&LogicSystem::FileResume, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);


	_fun_callbacks[ID_FILE_ACCEPT_REQ] = std::bind(&LogicSystem::FileAccept, this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3);

	_fun_callbacks[ID_FILE_META_RSP] = std::bind(&LogicSystem::FileMetaRsp, this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3);

	_fun_callbacks[ID_FILE_META_RSP] =
		std::bind(&LogicSystem::FileMetaAck, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3);

	_fun_callbacks[ID_FILE_META_RSP] = std::bind(&LogicSystem::ForwardFileMetaRsp,
		this, std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3);

}



void LogicSystem::Heartbeat(std::shared_ptr<CSession> session,
	const short&, const std::string&) {
	// 回复心跳包
	session->Send("", ID_HEARTBEAT_RSP);
	//session->ResetHeartbeat();               // ★ 调用会话层复位计时器
}



void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is  " << uid << " user token  is "
		<< token << endl;

	Json::Value  rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});

	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return ;
	}

	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return ;
	}

	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["back"] = user_info->back;
	rtvalue["icon"] = user_info->icon;

	//从数据库获取申请列表
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid,apply_list);
	if (b_apply) {
		for (auto & apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["status"] = apply->_status;
			rtvalue["apply_list"].append(obj);
		}
	}

	//获取好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj);
	}


	srand(static_cast<unsigned int>(time(nullptr)));
	std::vector<std::string> names = {
	"Alice", "Bob", "Charlie", "David", "Eve",
	"Frank", "Grace", "Heidi", "Ivan", "Judy",
	"Mallory", "Niaj", "Olivia", "Peggy", "Rupert"
	};
	//添加10个测试好友数据
	//for (int i = 1; i <= 10; ++i) {
	//	Json::Value test_friend;
	//	// 随机取一个名字
	//	std::string random_name = names[rand() % names.size()];
	//	test_friend["name"] = random_name;
	//	test_friend["uid"] = 10000 + i; // 给测试好友一个假uid，比如从10001开始
	//	// 生成1到5之间的随机数
	//	int random_icon_id = rand() % 5 + 1;
	//	// 拼接头像路径
	//	test_friend["icon"] = ":/res/head_" + std::to_string(random_icon_id) + ".jpg";
	//	rtvalue["friend_list"].append(test_friend);
	//}



	auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
	//将登录数量增加
	auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;
	if (!rd_res.empty()) {
		count = std::stoi(rd_res);
	}

	count++;
	auto count_str = std::to_string(count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);


	std::string online_key = "user_online_status_" + uid_str;
	// 设置为在线状态
	RedisMgr::GetInstance()->Set(online_key, "1");

	std::cout << "User " << uid << " logged in successfully." << std::endl;


	//session绑定用户uid
	session->SetUserId(uid);
	//为用户设置登录ip server的名字
	std::string  ipkey = USERIPPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(ipkey, server_name);
	//uid和session绑定管理,方便以后踢人操作
	UserMgr::GetInstance()->SetUserSession(uid, session);



	return;
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["uid"].asString();
	std::cout << "user SearchInfo uid is  " << uid_str << endl;

	Json::Value  rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = isPureDigit(uid_str);
	if (b_digit) {
		GetUserByUid(uid_str, rtvalue);
	}
	else {
		GetUserByName(uid_str, rtvalue);
	}
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto back = root["back"].asString();
	auto touid = root["touid"].asInt();
	std::cout << "user login uid is  " << uid << " applyname  is "
		<< applyname << " back is " << back << " touid is " << touid << endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}


	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, uid, apply_info);
	//直接通知对方有申请消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			if (b_info)
			{
				notify["icon"] = apply_info->icon;
				notify["back"] = back;
			}
		
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}
		return ;
	}

	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	if (b_info) {
		add_req.set_icon(apply_info->icon);
	}

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value,add_req);

}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["icon"] = user_info->icon;
		rtvalue["uid"] = touid;
		rtvalue["back"] = back_name;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}


	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);

	//更新数据库添加好友
	MysqlMgr::GetInstance()->AddFriend(uid, touid,back_name);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["icon"] = user_info->icon;
				notify["back"] = back_name;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}


			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}

		return ;
	}


	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value  arrays = root["text_array"];
	
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});


	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}
		return ;
	}


	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto *text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}


	//发送通知 todo...
	ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}


// isPureDigit作用：判断字符串是否全为数字
bool LogicSystem::isPureDigit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto icon = root["icon"].asString();
		auto back = root["back"].asString();
		std::cout << "Redis 查到  ： user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email <<" icon is " << icon << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["icon"] = icon;
		rtvalue["back"] = back;
		return;
	}

	auto uid = std::stoi(uid_str);
	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(uid);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["icon"] = user_info->icon;
	redis_root["back"] = user_info->back;
	std::cout << "mysql 查到  ： user  uid is  " << uid << " name  is "
		<< user_info->name << " pwd is " << user_info->pwd << " email is " << user_info->email << " icon is " << user_info->icon << endl;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["icon"] = user_info->icon;
	rtvalue["back"] = user_info->back;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = NAME_INFO + name;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto icon = root["icon"].asString();
		auto back = root["back"].asString();
		std::cout << "Redis 查到 ：user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["icon"] = icon;
		rtvalue["back"] = back;
		return;
	}

	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(name);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["icon"] = user_info->icon;
	redis_root["back"] = user_info->back;
	std::cout << "mysql 查到  ： user  uid is  " << user_info->uid << " name  is "
		<< name << " pwd is " << user_info->pwd << " email is " << user_info->email << " icon is " << user_info->icon << endl;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	
	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["icon"] = user_info->icon;
	rtvalue["back"] = user_info->back;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
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
		userinfo->back = root["back"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {
		//redis中没有则查询mysql
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;
		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["icon"] = userinfo->icon;
		redis_root["back"] = userinfo->back;
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &list) {
	//从mysql获取好友申请列表
	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {
	//从mysql获取好友列表
	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}
