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
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


//������
enum ErrorCodes {
	Success = 0,          
	Error_Json = 1001,
	RPCFailed = 1002,
};

