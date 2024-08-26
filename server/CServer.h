#pragma once
#include "const.h"


class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);
		//io_context��һ�������ģ����¼��ĵ��������ڵײ㲻����ѯ
	void Start();
private:
	tcp::acceptor _acceptor;   //��������������ܶԶ˵�����
	net::io_context& _ioc;      //������
	tcp::socket _socket;       //���ܶԶ˵�������Ϣ�������ڸ���
};

