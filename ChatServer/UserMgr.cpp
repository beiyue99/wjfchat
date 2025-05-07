#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
// 析构函数，清空 uid 到 session 的映射表
UserMgr::~UserMgr() {
	_uid_to_session.clear();
}

// 根据用户 uid 获取对应的会话指针
std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx); // 加锁，保证线程安全
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		return nullptr;
	}

	return iter->second;
}

// 将指定 uid 与其对应的 session 对象关联，存入映射表
void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(_session_mtx); // 加锁
	_uid_to_session[uid] = session;
}

// 移除某个用户的会话映射
void UserMgr::RmvUserSession(int uid)
{
	auto uid_str = std::to_string(uid);

	// 这里需要优化  ++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	auto key = USERIPPREFIX + uid_str;
	auto selfName = ConfigMgr::Inst().GetValue("SelfServer", "Name");



	////auto sha = ConfigMgr::Inst()["LuaSHA"].GetValue("CompareDel");
	//auto sha = ConfigMgr::Inst().GetValue("LuaSHA","CompareDel");
	//// 只有当 key 的值仍是本机时才删除
	//RedisMgr::GetInstance()->EvalSha(sha, { key }, { selfName });

	// ★ 修改：通过接口拿 SHA
	//const std::string& sha = RedisMgr::GetInstance()->CompareDelSha();
	//RedisMgr::GetInstance()->EvalSha(sha, { key }, { selfName });

	{
		std::lock_guard<std::mutex> lock(_session_mtx); // 加锁
		_uid_to_session.erase(uid); // 删除本地映射
	}
}

// 构造函数：无特殊初始化
UserMgr::UserMgr()
{

}
