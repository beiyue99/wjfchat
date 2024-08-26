#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include<QString>
#include<QUrl>
#include<QObject>
#include<QNetworkAccessManager>
#include<QJsonObject>
#include<QJsonDocument>






class HttpMgr:public QObject,public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    ~HttpMgr();   //因为基类智能指针释放T(HttpMgr)时会调用该方法，所以设置为共有
    //但是基类也会new一个T(HttpMgr)，因此构造函数也需要设置为共有，单着不符合初衷，因此引入友元
private:
    friend class Singleton<HttpMgr>;
    HttpMgr();
    QNetworkAccessManager _manager;  //qt中的网络管理者
    void PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod);
private slots:
    void slot_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);
signals:
    void sig_http_finish(ReqId id,QString res,ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqId id,QString res,ErrorCodes err);
};

#endif // HTTPMGR_H










