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
    _get_handlers[path](con);    //调用处理器处理
    return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
    _get_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req";
        });
}
