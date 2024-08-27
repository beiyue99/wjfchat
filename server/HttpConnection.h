#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket);
	void Start();  //���ڼ�����д�¼�
private:
	void CheckDeadline();   //��ʱ���
	void WriteResponse();  //�յ����ݺ��Ӧ����
	void HandleReq(); //��������
	void PreParseGetParam();// 
	����Ĳ�������
	tcp::socket _socket;
	beast::flat_buffer _buffer{ 8192 }; //�������ݵ�buffer
	http::request<http::dynamic_body> _request; //���նԷ�������
	http::response<http::dynamic_body> _response; //�ظ��Է�
	net::steady_timer deadline_{
		_socket.get_executor(),std::chrono::seconds(60)//60�볬ʱ 
	//��ʱ���ڵײ��¼�ѭ������Ҫ������
	};
	std::string _get_url;   //����url
	std::unordered_map<std::string, std::string> _get_params;   
};

