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
    // ע��һ�� GET ����������·��Ϊ "/get_test"
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        // ʹ�� beast::ostream ����Ӧ����д�뵽 HTTP ��Ӧ����
        beast::ostream(connection->_response.body()) << "receive get_test req\r\n";

        // ���� GET ����Ĳ����������������Ͳ���ֵд�뵽��Ӧ����
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " << "value is " << elem.second << std::endl;
        }
        });

    // ע��һ�� POST ����������·��Ϊ "/get_varifycode"
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        // �� HTTP �������л�ȡ���ݲ�ת��Ϊ�ַ���
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;

        // ���� HTTP ��Ӧ����������Ϊ JSON
        connection->_response.set(http::field::content_type, "text/json");

        // JSON ��صĴ������������ͽ�����
        Json::Value root;  // ���ڴ洢���ص� JSON ����
        Json::Reader reader; // JSON �����������ڽ��������е� JSON ����
        Json::Value src_root; // ���ڴ洢������� JSON ����

        // �����������е� JSON ���ݣ���ʧ���򷵻ش�����Ϣ
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json; // ���ô�����
            std::string jsonstr = root.toStyledString(); // ת��Ϊ��ʽ���� JSON �ַ���
            beast::ostream(connection->_response.body()) << jsonstr; // ��������Ϣд����Ӧ��
            return true;
        }

        // ��� JSON �������Ƿ���� "email" �ֶΣ���ȱʧ�򷵻ش�����Ϣ
        if (!src_root.isMember("email")) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json; // ���ô�����
            std::string jsonstr = root.toStyledString(); // ת��Ϊ��ʽ���� JSON �ַ���
            beast::ostream(connection->_response.body()) << jsonstr; // ��������Ϣд����Ӧ��
            return true;
        }

        // ��ȡ "email" �ֶε�ֵ������ӡ������̨
        auto email = src_root["email"].asString();
        //ʹ�� gRPC �ͻ���������֤��
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        std::cout << "email is " << email << std::endl;

        // ���سɹ��� JSON ��Ӧ������ email �ʹ����� 0
        root["error"] = rsp.error();
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString(); // ת��Ϊ��ʽ���� JSON �ַ���
        beast::ostream(connection->_response.body()) << jsonstr; // ����Ӧ����д����Ӧ��
        return true;
        });
}
