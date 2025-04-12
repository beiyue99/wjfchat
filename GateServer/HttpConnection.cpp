#include "HttpConnection.h"
#include "LogicSystem.h"

// 初始化 socket
HttpConnection::HttpConnection(boost::asio::io_context& ioc) : _socket(ioc) {}

// 启动连接，异步读取请求数据
void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try {
			if (ec) {
				std::cout << "http read err is" << ec.what() << std::endl;
				return;
			}
			boost::ignore_unused(bytes_transferred);
			self->HandleReq();     // 请求处理入口
			self->CheckDeadline(); // 开始检查超时
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}

// 将字符转换为十六进制字符
unsigned char ToHex(unsigned char x) {
	return x > 9 ? x + 55 : x + 48;
}

// 从十六进制字符还原为普通字符
unsigned char FromHex(unsigned char x) {
	if (x >= 'A' && x <= 'Z') return x - 'A' + 10;
	if (x >= 'a' && x <= 'z') return x - 'a' + 10;
	if (x >= '0' && x <= '9') return x - '0';
	assert(0);
	return 0;
}

// 对 URL 进行编码
std::string UrlEncode(const std::string& str) {
	std::string strTemp;
	for (size_t i = 0; i < str.length(); i++) {
		if (isalnum((unsigned char)str[i]) ||
			str[i] == '-' || str[i] == '_' ||
			str[i] == '.' || str[i] == '~') {
			strTemp += str[i];
		}
		else if (str[i] == ' ') {
			strTemp += "+";
		}
		else {
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

// 对 URL 编码内容进行解码
std::string UrlDecode(const std::string& str) {
	std::string strTemp;
	for (size_t i = 0; i < str.length(); i++) {
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%') {
			assert(i + 2 < str.length());
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

// 解析 GET 请求的参数
void HttpConnection::PreParseGetParam() {
	auto uri = _request.target();
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}
	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);

	while (!query_string.empty()) {
		auto pos = query_string.find('&');
		std::string pair = (pos != std::string::npos) ? query_string.substr(0, pos) : query_string;
		auto eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			auto key = UrlDecode(pair.substr(0, eq_pos));
			auto value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		if (pos == std::string::npos) break;
		query_string.erase(0, pos + 1);
	}
}

// 处理 HTTP 请求（GET 或 POST）
void HttpConnection::HandleReq() {
	_response.version(_request.version());
	_response.keep_alive(false);

	// 处理 GET 请求
	if (_request.method() == http::verb::get) {
		PreParseGetParam();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}

	if (_request.method() == http::verb::post) {
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

// 检查连接是否超时
void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	deadline_.async_wait([self](beast::error_code ec) {
		if (!ec) {
			self->_socket.close(ec);
		}
		});
}

// 将响应写回客户端
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());

	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		self->deadline_.cancel();
		});
}
