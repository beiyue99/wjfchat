#pragma once
#include "const.h"




class HttpConnection;
//������ɵ��ö���HttpHandler   ����ֵΪvoid������Ϊһ��http���Ӷ���
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;


class LogicSystem :public Singleton<LogicSystem> {
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	//���յ�һ��HTTP GET ����ʱ�����������·�� (path) ���Ҳ�������Ӧ�Ĵ�����������
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	//���յ�һ��HTTP POST ����ʱ�����������·�� (path) ���Ҳ�������Ӧ�Ĵ�����������
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	//ע��һ�����ڴ����ض� URL ·���� HTTP GET ��������
	void RegGet(std::string, HttpHandler handler);
	//ע��һ�����ڴ����ض� URL ·���� HTTP POST ��������
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	//ע�ᴦ��HTTP GET ���󣬽��� URL ���������ظ��ͻ��ˡ�
	//ע�ᴦ��HTTP POST ���󣬽�����Ϣ���е� JSON ���ݣ�����������������Ӧ����Ӧ��
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};


