#pragma once

#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <unordered_map>
#include "data.h"

// 定义消息处理函数类型：参数为 session、消息 ID、消息内容
typedef function<void(shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FunCallBack;

// LogicSystem 是逻辑处理核心模块，负责后台线程异步消费消息、分发消息回调、业务逻辑统一入口
// 采用单例模式，保证全局只有一个逻辑系统实例
class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;

public:
	~LogicSystem(); // 析构函数，关闭线程并清理资源

	// 向消息队列中投递一条逻辑消息，由后台线程统一处理
	void PostMsgToQue(shared_ptr<LogicNode> msg);

private:
	LogicSystem(); // 构造函数，初始化线程和注册消息回调

	// 后台线程执行函数，不断处理消息队列中的逻辑消息
	void DealMsg();

	// 注册所有逻辑消息对应的回调函数
	void RegisterCallBacks();

	// 登录处理逻辑：校验 token、拉取用户基本信息、申请列表、好友列表
	void LoginHandler(shared_ptr<CSession> session, const short& msg_id, const string& msg_data);

	// 用户搜索逻辑：支持按 UID 或用户名搜索
	void SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);

	// 添加好友申请处理逻辑：写入数据库、通知对方、发起 gRPC 请求
	void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);

	// 好友认证处理逻辑：同意/拒绝申请，建立好友关系并通知对方
	void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);

	// 文本聊天消息处理逻辑：分发聊天消息给目标用户
	void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);

	// 判断字符串是否为纯数字（用于区分 uid 和用户名搜索）
	bool isPureDigit(const std::string& str);

	// 通过 UID 获取用户信息，优先查 Redis，未命中则查数据库
	void GetUserByUid(std::string uid_str, Json::Value& rtvalue);

	// 通过用户名获取用户信息，优先查 Redis，未命中则查数据库
	void GetUserByName(std::string name, Json::Value& rtvalue);

	// 获取用户基础信息（用于登录、好友信息展示等），自动缓存到 Redis
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

	// 获取指定用户的好友申请列表（用于登录时拉取申请）
	bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);

	// 获取指定用户的好友列表（用于登录或聊天）
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);

private:
	std::thread _worker_thread; // 后台处理逻辑消息的线程
	std::queue<shared_ptr<LogicNode>> _msg_que; // 消息队列，缓存待处理的逻辑消息
	std::mutex _mutex; // 队列访问互斥锁
	std::condition_variable _consume; // 条件变量，用于线程间通知
	bool _b_stop; // 停服标志，用于安全退出后台线程

	std::map<short, FunCallBack> _fun_callbacks; // 消息 ID → 回调函数的映射表
};
