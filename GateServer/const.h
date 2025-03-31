#pragma once
#include<boost/beast/http.hpp>
#include<boost/beast.hpp>
#include<boost/asio.hpp>
#include<memory>
#include<iostream>
#include "Singleton.h"
#include<functional>
#include<map>
#include<unordered_map>
#include<json/json.h>
#include<json/value.h>
#include<json/reader.h>
#include<boost/filesystem.hpp>
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/ini_parser.hpp>
#include<queue>
#include<mutex>
#include<atomic>
#include<condition_variable>

#include "hiredis.h"
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


//错误码
enum ErrorCodes {
	Success = 0,          
	Error_Json = 1001, // JSON 解析错误
	RPCFailed = 1002, // RPC 调用失败
	VarifyExpired = 1003, // 验证码过期
	VarifyCodeErr = 1004, // 验证码错误
	UserExist = 1005, // 用户已存在
	PasswdErr = 1006, // 密码错误
	EmailNotMatch = 1007, // 邮箱不匹配
	PasswdUpFailed = 1008, // 密码修改失败
	PasswdInvalid = 1009, // 密码不合法
};


// Defer类, 用于延迟执行函数, 用法: Defer defer([](){});defer在作用域结束时执行传入的函数
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

#define CODEPREFIX "code_"      // 验证码前缀