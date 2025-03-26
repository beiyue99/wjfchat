#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "const.h"
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel; //grpc通道
using grpc::Status; //grpc状态	
using grpc::ClientContext; //grpc客户端上下文	

using message::AddFriendReq; //添加好友请求	
using message::AddFriendRsp; //添加好友响应	

using message::AuthFriendReq; //认证好友请求	
using message::AuthFriendRsp; //认证好友响应	

using message::GetChatServerRsp; //获取聊天服务器响应	
using message::LoginRsp; //登录响应	
using message::LoginReq; //登录请求	
using message::ChatService; //聊天服务

using message::TextChatMsgReq; //文本聊天请求
using message::TextChatMsgRsp;	//文本聊天响应
using message::TextChatData; //文本聊天数据


class ChatConPool {
public:
	ChatConPool(size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());
			connections_.push(ChatService::NewStub(channel));
		}
	}

	~ChatConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<ChatService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
			});
		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	void returnConnection(std::unique_ptr<ChatService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

private:
	atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<ChatService::Stub> > connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient() {
	}

	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req); //通知添加好友
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req); //通知认证好友
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo); //获取用户基本信息
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, 
		const TextChatMsgReq& req, const Json::Value& rtvalue); //通知文本聊天消息
	
private:
	ChatGrpcClient(); //获取配置信息,初始化grpc连接池
	unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools; 
	//grpc连接池,根据server_ip存储,每个server_ip对应一个连接池
};



