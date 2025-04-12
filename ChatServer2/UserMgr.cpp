#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"

// 析构函数，清空 uid 到 session 的映射表
UserMgr::~UserMgr() {
	_uid_to_session.clear();
}

// 根据用户 uid 获取对应的会话指针
// 如果该 uid 不存在于当前服务器的会话映射表中，则返回 nullptr
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

	// 注意：如果当前用户重新登录到了其他服务器，那么本服务器上再删 redis 中的记录就不安全
	// 所以此处注释掉了删除 redis 中 IP 记录的代码
	// RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	{
		std::lock_guard<std::mutex> lock(_session_mtx); // 加锁
		_uid_to_session.erase(uid); // 删除本地映射
	}
}

// 构造函数：无特殊初始化
UserMgr::UserMgr()
{

}
