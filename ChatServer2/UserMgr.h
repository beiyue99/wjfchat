#pragma once

#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class CSession; // 前向声明：代表用户会话连接类

// UserMgr 是一个用户会话管理类，用于维护 uid 到 CSession 的映射关系
// 采用单例模式管理，确保全局唯一性
class UserMgr : public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>; // 允许 Singleton 模板访问其私有构造函数
public:
	~UserMgr();

	// 根据用户 uid 获取对应的会话 session，如果不存在则返回 nullptr
	std::shared_ptr<CSession> GetSession(int uid);

	// 设置用户 uid 与对应的会话 session 的映射关系
	void SetUserSession(int uid, std::shared_ptr<CSession> session);

	// 移除指定 uid 的会话 session 映射
	void RmvUserSession(int uid);

private:
	UserMgr(); // 私有构造函数，禁止外部直接构造

	std::mutex _session_mtx; // 线程安全的互斥锁，保护下面的 session 映射
	std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session; // 用户 uid 到 CSession 的映射表
};
