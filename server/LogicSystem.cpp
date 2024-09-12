#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
LogicSystem::~LogicSystem()
{
}


bool LogicSystem::HandleGet(std::string path , std::shared_ptr<HttpConnection> con)
{
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
    _get_handlers[path](con);    //调用处理器处理
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_post_handlers.find(path) == _post_handlers.end()) {
        return false;
    }

    _post_handlers[path](con);
    return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
    _get_handlers.insert(make_pair(url, handler));
}


void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
    _post_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
    // 注册一个 GET 请求处理函数，路径为 "/get_test"
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        // 使用 beast::ostream 将响应内容写入到 HTTP 响应体中
        beast::ostream(connection->_response.body()) << "receive get_test req\r\n";

        // 遍历 GET 请求的参数，并将参数名和参数值写入到响应体中
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " << "value is " << elem.second << std::endl;
        }
        });

    // 注册一个 POST 请求处理函数，路径为 "/get_varifycode"
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        // 从 HTTP 请求体中获取数据并转换为字符串
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;

        // 设置 HTTP 响应的内容类型为 JSON
        connection->_response.set(http::field::content_type, "text/json");

        // JSON 相关的处理：定义根对象和解析器
        Json::Value root;  // 用于存储返回的 JSON 数据
        Json::Reader reader; // JSON 解析器，用于解析请求中的 JSON 数据
        Json::Value src_root; // 用于存储解析后的 JSON 数据

        // 解析请求体中的 JSON 数据，若失败则返回错误信息
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json; // 设置错误码
            std::string jsonstr = root.toStyledString(); // 转换为格式化的 JSON 字符串
            beast::ostream(connection->_response.body()) << jsonstr; // 将错误信息写入响应体
            return true;
        }

        // 检查 JSON 数据中是否包含 "email" 字段，若缺失则返回错误信息
        if (!src_root.isMember("email")) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json; // 设置错误码
            std::string jsonstr = root.toStyledString(); // 转换为格式化的 JSON 字符串
            beast::ostream(connection->_response.body()) << jsonstr; // 将错误信息写入响应体
            return true;
        }

        // 获取 "email" 字段的值，并打印到控制台
        auto email = src_root["email"].asString();
        //使用 gRPC 客户端请求验证码
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        std::cout << "email is " << email << std::endl;

        // 返回成功的 JSON 响应，包含 email 和错误码 0
        root["error"] = rsp.error();
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString(); // 转换为格式化的 JSON 字符串
        beast::ostream(connection->_response.body()) << jsonstr; // 将响应数据写入响应体
        return true;
        });
}
