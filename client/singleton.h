#ifndef SINGLETON_H
#define SINGLETON_H
#include"global.h"



//单例模板类，会根据模板参数实例出不同的单例


template <typename T>
class Singleton
{
protected:
    Singleton()=default;  //希望基类继承的时候可以构造，所以设置为保护
    Singleton(const Singleton<T>&)=delete;
    Singleton& operator=(const Singleton<T>&)=delete;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance(){
         static std::once_flag s_flag;
         std::call_once(s_flag,[&](){
             _instance=std::shared_ptr<T>(new T);
         });
         return _instance;
    }
    void PrintAddress(){
        std::cout<<_instance.get()<<std::endl;
    }
    ~Singleton(){
        std::cout<<"this is Singleton destruct!"<<std::endl;
    }

};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance=nullptr;


#endif // SINGLETON_H
