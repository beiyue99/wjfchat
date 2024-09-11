#ifndef GLOBAL_H
#define GLOBAL_H
#include<QWidget>
#include<functional>
#include"QStyle"
#include<QRegularExpression>
#include<memory>
#include<iostream>
#include<mutex>
#include<QByteArray>
#include<QNetworkReply>
#include<QJsonObject>
#include<QDir>
#include<QSettings>



//    用来刷新qss
extern std::function<void(QWidget*)>repolish;


//请求类型
enum ReqId{
    ID_GET_VARIFY_CODE=1001,//获取验证码
    ID_REG_USER=1002,//注册用户
};

//定义请求的模块
enum Modules{
    REGISTERMOD=0,  //注册模块
};

//错误码
enum ErrorCodes{
    SUCCESS=0,
    ERR_JSON=1, //json解析失败
    ERR_NETWORK=2,//网络错误
};



//访问gate_url的前缀
extern QString gate_url_prefix;


#endif // GLOBAL_H















