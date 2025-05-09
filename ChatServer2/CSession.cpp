#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "RedisMgr.h"
#include "LogicSystem.h"

// 构造函数：创建 Session 对象，并生成唯一 Session ID
CSession::CSession(boost::asio::io_context& io_context, CServer* server)
	: _socket(io_context), _server(server), _b_close(false), 
	_b_head_parse(false), _user_uid(0),_hb_timer(io_context)  {  // ★ 新增

	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_session_id = boost::uuids::to_string(a_uuid); // 生成唯一会话 ID
	_recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN); // 创建消息头节点缓存区
}


// 实现新增的函数
void CSession::ResetHeartbeat() {
	_hb_timer.expires_after(std::chrono::seconds(HB_TIMEOUT));
	auto self = shared_from_this();
	_hb_timer.async_wait([self](const boost::system::error_code& ec) {
		if (ec == boost::asio::error::operation_aborted) return; // 被取消
		if (!ec) {
			std::cout << "Heartbeat timeout, uid=" << self->_user_uid << std::endl;
			self->Close();   // 触发下线
			self->_server->ClearSession(self->_session_id); // ② ★ 新增 补这一行
		}
		});
}



// 析构函数
CSession::~CSession() {
	std::cout << "~CSession destruct" << endl;
}

// 获取底层 socket
tcp::socket& CSession::GetSocket() {
	return _socket;
}

// 获取当前 Session ID
std::string& CSession::GetSessionId() {
	return _session_id;
}

// 设置当前登录用户的 UID
void CSession::SetUserId(int uid) {
	_user_uid = uid;
}

// 获取当前登录用户 UID
int CSession::GetUserId() {
	return _user_uid;
}

// 启动 Session，开始读取消息头
void CSession::Start() {
	AsyncReadHead(HEAD_TOTAL_LEN); // 监听头部长度
	ResetHeartbeat();        // ★ 新增   启动定时器
}

// 向客户端发送消息（string 版本）
void CSession::Send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {  // 队列满了,直接返回
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}
	_send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
	if (send_que_size > 0) return; // 正在发送中
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

// 向客户端发送消息（char* buffer 版本）
void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}
	_send_que.push(make_shared<SendNode>(msg, max_length, msgid));
	if (send_que_size > 0) return;
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

// 关闭连接
void CSession::Close() {
	_socket.close();
	_b_close = true;
	_hb_timer.cancel();      // ★ 新增
}

// 获取共享指针自身
std::shared_ptr<CSession> CSession::SharedSelf() {
	return shared_from_this();
}

// 异步读取消息体数据
void CSession::AsyncReadBody(int total_len) {
	// 获取当前会话的 shared_ptr，确保在异步操作中生命周期安全
	auto self = shared_from_this();

	// 异步读取 total_len 字节（也就是消息体长度）
	asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			// 1. 检查读取是否出错
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << endl;
				// 如果出错，关闭连接并移除会话
				Close();
				_server->ClearSession(_session_id);
				return;
			}

			// 2. 判断读取长度是否满足预期
			if (bytes_transfered < total_len) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< total_len << "]" << endl;
				Close();
				_server->ClearSession(_session_id);
				return;
			}

			// 3. 将读取到的内容拷贝到消息节点中
			memcpy(_recv_msg_node->_data, _data, bytes_transfered);           // 拷贝数据
			_recv_msg_node->_cur_len += bytes_transfered;                     // 更新已读长度
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';         // 设置结束符（安全）

			// 4. 将消息封装成 LogicNode 投递到逻辑系统队列（异步解耦）
			LogicSystem::GetInstance()->PostMsgToQue(
				make_shared<LogicNode>(shared_from_this(), _recv_msg_node));

			ResetHeartbeat();                                   // 新增 ★这里复位心跳定时器
			// 5. 继续读取下一条消息头，保持会话持续接收
			AsyncReadHead(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}


// 异步读取消息
void CSession::AsyncReadHead(int total_len) {
	// 创建 shared_ptr 保证回调期间 this 不会被销毁
	auto self = shared_from_this();

	// 开始异步读取固定长度的消息头（HEAD_TOTAL_LEN 字节）
	asyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			// 如果发生读取错误（客户端断开等），清理 session
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << endl;


				// 更新 Redis 中用户在线状态为 0（下线）
				std::string online_key = "user_online_status_" + std::to_string(GetUserId());
				RedisMgr::GetInstance()->Set(online_key, "0");

				Close(); // 关闭 socket
				_server->ClearSession(_session_id); // 从服务器移除该连接
				return;
			}

			// 如果读取到的数据长度不够，说明客户端异常或恶意行为，直接断开
			if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total [" << HEAD_TOTAL_LEN << "]" << endl;
				// 更新 Redis 中用户在线状态为 0（下线）
				std::string online_key = "user_online_status_" + std::to_string(GetUserId());
				RedisMgr::GetInstance()->Set(online_key, "0");

				Close();
				_server->ClearSession(_session_id);
				return;
			}

			// 清空上一次残留的头部数据
			_recv_head_node->Clear();

			// 将读取的数据复制到 _recv_head_node 中保存
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			// 从头部数据中解析出 msg_id（前两个字节）
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id); // 转为本地字节序

			// 校验 msg_id 合法性
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << endl;
				_server->ClearSession(_session_id);
				return;
			}

			// 解析出 msg_len（接下来的两个字节）
			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

			// 校验消息体长度是否合法
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << endl;
				_server->ClearSession(_session_id);
				return;
			}

			// 创建消息体节点，准备读取消息体
			_recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);

			// 启动读取消息体流程
			AsyncReadBody(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}


// 处理异步写完成的回调函数
void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	try {
		if (!error) {
			// 写操作成功
			std::lock_guard<std::mutex> lock(_send_lock);

			// 当前消息已经发送完成，移除队首消息
			_send_que.pop();

			// 如果还有待发送的消息，则继续发送下一条
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();

				// 发起下一次异步写操作
				boost::asio::async_write(
					_socket,                                      // 当前连接 socket
					boost::asio::buffer(msgnode->_data, msgnode->_total_len), // 要发送的数据 buffer
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self) // 回调绑定
				);
			}
		}
		else {
			// 写操作失败，打印错误信息
			std::cout << "handle write failed, error is " << error.what() << endl;

			// 关闭当前连接
			Close();

			// 通知服务器移除该 session（释放资源）
			_server->ClearSession(_session_id);
		}
	}
	catch (std::exception& e) {
		// 捕获异常防止程序崩溃
		std::cerr << "Exception code : " << e.what() << endl;
	}
}


// 封装读取完整长度的数据
void CSession::asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler) {
	::memset(_data, 0, MAX_LENGTH); // 清空 buffer
	asyncReadLen(0, maxLength, handler);
}

// 异步分段读取直到满足 total_len
void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler) {

	// 1. 获取 shared_ptr，保证当前 Session 对象在异步回调中不会被释放
	auto self = shared_from_this();

	// 2. 异步读取数据：从 socket 中读入数据，写入 _data 缓冲区的指定位置
	//    读的位置是从 _data + read_len 开始，长度是剩余未读的 total_len - read_len 字节
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),

		// 3. 这是读取完成后的回调函数
		[read_len, total_len, handler, self](const boost::system::error_code& ec, std::size_t bytesTransfered) {

			// 4. 如果发生错误（比如连接断开），直接调用外部的 handler，并传回当前已读取的总长度
			if (ec) {
				handler(ec, read_len + bytesTransfered);
				return;
			}

			// 5. 如果当前读取的总长度已经满足 total_len，说明消息读取完成
			if (read_len + bytesTransfered >= total_len) {
				handler(ec, read_len + bytesTransfered);
				return;
			}

			// 6. 否则说明还没读完，继续递归调用 asyncReadLen 接着读取剩余部分
			self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
		});
}


// LogicNode 构造函数：用于包装逻辑消息，转入逻辑线程处理
LogicNode::LogicNode(shared_ptr<CSession> session, shared_ptr<RecvNode> recvnode)
	: _session(session), _recvnode(recvnode) {
}
