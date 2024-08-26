#include "HttpConnection.h"
#include "LogicSystem.h"
HttpConnection::HttpConnection(tcp::socket socket) :_socket(std::move(socket)) {

}

void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try {
			if (ec) {
				std::cout << "http read err is" << ec.what() << std::endl;
				return;
			}
			boost::ignore_unused(bytes_transferred);//忽略参数未使用的警告
			self->HandleReq();
			self->CheckDeadline();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}


void HttpConnection::HandleReq() {
	_response.version(_request.version());//设置版本
	_response.keep_alive(false);
	if (_request.method() == http::verb::get) {  //如果是get请求
		bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer"); //告诉对方是什么服务
		WriteResponse();
		return;
	}
}

void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transferred) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);  //关闭发送端
		self->deadline_.cancel();  //取消定时器
		});
}

void HttpConnection::CheckDeadline() {
	deadline_.async_wait([self = shared_from_this()](beast::error_code ec) {
		if (!ec) {
			self->_socket.close(ec);
		}
		});
}