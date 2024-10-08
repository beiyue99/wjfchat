#pragma once
#include "const.h"


template <typename T>
class Singleton
{
protected:
    Singleton() = default;  //希望基类继承的时候可以构造，所以设置为保护
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
            });
        return _instance;
    }
    //打印实例地址的函数
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;  
    }
    ~Singleton() {
        std::cout << "this is Singleton destruct!" << std::endl;
    }

};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;