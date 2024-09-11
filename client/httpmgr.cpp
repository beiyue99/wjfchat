#include "httpmgr.h"

HttpMgr::~HttpMgr()
{

}

HttpMgr::HttpMgr()
{
    connect(this,&HttpMgr::sig_http_finish,this,&HttpMgr::slot_http_finish);
}



void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    //将json对象封装到json文档，然后调用tojson转化为json格式字符串，储存在data
    QByteArray data=QJsonDocument(json).toJson();
    QNetworkRequest request(url);
    //url 是一个 QUrl 对象，表示请求的目标地址，通常是一个网络 URL（例如，http://example.com）。


    //设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(data.length()));


    auto self =shared_from_this();  //获取一个 shared_ptr 指向当前对象
    QNetworkReply* reply=_manager.post(request,data); //发送post请求，携带的数据是json对象
    //lambda表达式会用到当前类的数据，所以要保证触发这个回调之前这个httpmgr对象不能被删除
    connect(reply,&QNetworkReply::finished,[self=std::move(self),reply,req_id,mod](){
        //处理错误情况
        if(reply->error()!=QNetworkReply::NoError){
            qDebug()<<reply->errorString();
            //发送信号通知完成
            emit self->sig_http_finish(req_id,"",ErrorCodes::ERR_NETWORK,mod);
            reply->deleteLater();
            return ;
        }
        //无错误
        QString res=reply->readAll();
        //发送信号通知完成,参数就是服务器响应的数据
        emit self->sig_http_finish(req_id,res,ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod==Modules::REGISTERMOD){
        //发送信号通知指定模块的http响应结束了
        emit sig_reg_mod_finish(id,res,err);
    }
}

















