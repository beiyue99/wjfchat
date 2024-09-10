#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include<QString>
#include<QUrl>
#include<QObject>
#include<QNetworkAccessManager>
#include<QJsonObject>
#include<QJsonDocument>





//http管理类
class HttpMgr:public QObject,public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    ~HttpMgr();   //因为基类智能指针释放T(HttpMgr)时会调用该方法，所以设置为共有
    //但是基类也会new一个T(HttpMgr)，因此构造函数也需要设置为共有，单着不符合初衷，因此引入友元


     void PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod);
     //发送一个 HTTP POST 请求，将 JSON 数据发送到指定的 URL，然后处理服务器的响应结果（包括成功和失败情况），
     //并通过 sig_http_finish 信号通知其他模块请求的结果。

     //QUrl url: 请求的目标 URL
     //QJsonObject json: 要发送的 JSON 数据，将会作为 POST 请求的请求体。
     //ReqId req_id: 请求的唯一标识符，用于在请求完成后追踪请求结果。
     //Modules mod: 代表当前请求所属的模块，用于区分请求的来源或目的。
private:
    friend class Singleton<HttpMgr>;
    HttpMgr();
    QNetworkAccessManager _manager;  //qt中的网络管理者
private slots:
    void slot_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);
signals:
    void sig_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqId id,QString res,ErrorCodes err);
};

#endif // HTTPMGR_H










