#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),
_acceptor(ioc,tcp::endpoint(tcp::v4(),port)){

}

//void CServer::Start(){
//	auto self = shared_from_this();
//	_acceptor.async_accept(_socket, [self=std::move(self)](beast::error_code ec) {  
//		//_acceptor.async_accept �÷�������ʼ�����ͻ������ӣ�����һ���첽��������ɺ����ûص�����
//		try {
//			//����ͷ���������ӣ�����������������
//			if (ec) {
//				self->Start();
//				return;
//			}
//			//���������ӣ�������HttpConnection������������
//			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
//			//��������
//			self->Start();
//		}
//		catch (std::exception& exp) {
//
//		}
//		});
//}


void CServer::Start()
{
    auto self = shared_from_this();
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
    _acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
        try {
            //���������������ӣ���������������
            if (ec) {
                self->Start();
                return;
            }
            //���������ӣ�����HpptConnection�����������
            new_con->Start();
            //��������
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
        });
}