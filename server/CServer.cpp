#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),
_acceptor(ioc,tcp::endpoint(tcp::v4(),port)){

}

//void CServer::Start(){
//	auto self = shared_from_this();
//	_acceptor.async_accept(_socket, [self=std::move(self)](beast::error_code ec) {  
//		//_acceptor.async_accept 让服务器开始监听客户端连接，这是一种异步操作，完成后会调用回调函数
//		try {
//			//出错就放弃这个连接，继续监听其他连接
//			if (ec) {
//				self->Start();
//				return;
//			}
//			//创建新连接，并创建HttpConnection类管理这个连接
//			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
//			//继续监听
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
            //出错则放弃这个连接，继续监听新链接
            if (ec) {
                self->Start();
                return;
            }
            //处理新链接，创建HpptConnection类管理新连接
            new_con->Start();
            //继续监听
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
        });
}