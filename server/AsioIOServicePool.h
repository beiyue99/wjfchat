#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"





//io_context ������ά��һ��������У�Task Queue�������㽫 I/O �����������첽 socket �������ύ�� io_context ʱ��
// ����ᱻ��������С���Щ������������� I / O ��������ʱ�����������������û��Զ��������
//ÿ�ε��� io_context::run()�����᲻�ϴӶ�����ȡ������ִ�У�ֱ����������ִ����ϡ�
//io_context �������Щ���������״̬����ĳ���첽�������ʱ��io_context �Ὣ��Ӧ�Ļص����������������






class AsioIOServicePool :public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // ʹ�� round-robin �ķ�ʽ����һ�� io_service
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t                        _nextIOService;
};

