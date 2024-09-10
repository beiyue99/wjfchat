#include "LogicSystem.h"
#include "HttpConnection.h"
LogicSystem::~LogicSystem()
{
}


bool LogicSystem::HandleGet(std::string path , std::shared_ptr<HttpConnection> con)
{
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
    _get_handlers[path](con);    //���ô���������
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
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req\r\n";
        //ʹ�� beast::ostream ����Ӧд�뵽 HTTP ��Ӧ����
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " << " value is " << elem.second << std::endl;
        }
        });
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        //�� HTTP �����е���Ϣ�壨body��ת��Ϊһ�� std::string ���͵��ַ���
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        //���Է���Ӧ����Ӧ�� Content-Type ͷ���ֶΣ�������Ӧ����������Ϊ JSON ��ʽ
        Json::Value root;  // JSON ���ݵĻ�������
        Json::Reader reader;//���ڴ��ַ��������ж�ȡ JSON ���ݲ��������Ϊ Json::Value ����
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            //���ڽ� Json::Value ����ת��Ϊ��ʽ���� JSON �ַ��������ָ�ʽ�����ַ����Ǿ��пɶ��Ե�
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        if (!src_root.isMember("email")) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        auto email = src_root["email"].asString();
        std::cout << "email is " << email << std::endl;
        root["error"] = 0;
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
        });
}
