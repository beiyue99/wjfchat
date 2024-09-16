#pragma once
#include "const.h"


class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);
		//io_context��һ�������ģ����¼��ĵ��������ڵײ㲻����ѯ


	//�������������첽���չ��ܣ��Խ��ܿͻ����������󣬲�Ϊÿ�������Ӵ���һ�� HttpConnection ʵ�����й���
	void Start();
private:
	tcp::acceptor _acceptor;   //��������������ܶԶ˵�����
	net::io_context& _ioc;      //������
	//tcp::socket _socket;       
};

