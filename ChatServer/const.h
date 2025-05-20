#pragma once
#include <functional>


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //Json解析错误
	RPCFailed = 1002,  //RPC请求错误
	VarifyExpired = 1003, //验证码过期
	VarifyCodeErr = 1004, //验证码错误
	UserExist = 1005,       //用户已经存在
	PasswdErr = 1006,    //密码错误
	EmailNotMatch = 1007,  //邮箱不匹配
	PasswdUpFailed = 1008,  //更新密码失败
	PasswdInvalid = 1009,   //密码更新失败
	TokenInvalid = 1010,   //Token失效
	UidInvalid = 1011,  //uid无效
};


// Defer类
class Defer {
public:
	// 接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	// 析构函数中执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};

// MAX_LENGTH是接收数据的最大长度
#define MAX_LENGTH  1024*2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
	MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
	ID_SEARCH_USER_REQ = 1007, //用户搜索请求
	ID_SEARCH_USER_RSP = 1008, //搜索用户回包
	ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
	ID_ADD_FRIEND_RSP  = 1010, //申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
	ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息

	ID_HEARTBEAT_REQ = 1021,   // ★ 新增：客户端→服务器
	ID_HEARTBEAT_RSP = 1022,   // ★ 新增：服务器→客户端


	// ===== 文件收发 =====
	ID_FILE_META_REQ = 1031,   // 发送方→服务器：文件元数据
	ID_FILE_META_RSP = 1032,   // 服务器→发送方：元数据确认/断点信息
	ID_FILE_DATA_REQ = 1033,   // 发送方→服务器：二进制分片
	ID_FILE_DATA_RSP = 1034,   // 服务器→发送方：分片确认(可选)
	ID_FILE_FINISH_REQ = 1035,   // 发送方→服务器：全部分片已发完
	ID_FILE_FINISH_RSP = 1036,   // 服务器→发送/接收方：成功/失败
	ID_FILE_RESUME_REQ = 1037,   // 发送方→服务器：查询已收大小
	ID_FILE_RESUME_RSP = 1038,   // 服务器→发送方：返回 offset

	/* ===== 手动同意/拒绝 ===== */
	ID_FILE_ACCEPT_REQ = 1041,   // 接收端  -> 服务器（action = 1 接受 / 0 拒绝）
	ID_FILE_ACCEPT_RSP = 1042,   // 服务器 -> 发送端（透传或回执，见下文）
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"


