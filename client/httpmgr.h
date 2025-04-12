#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include "global.h"

// 网络请求管理类，负责发送 HTTP 请求并处理响应结果
class HttpMgr : public QObject,
                public Singleton<HttpMgr>,
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT

public:
    // 析构函数
    ~HttpMgr();

    // 发送 POST 请求（JSON 数据），会自动绑定请求 ID 和模块类型
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    // 构造函数（私有化，配合 Singleton 使用）
    HttpMgr();
    friend class Singleton<HttpMgr>;

    // Qt 的网络访问管理器，用于发送网络请求
    QNetworkAccessManager _manager;

public slots:
    // 槽函数：处理 HTTP 请求完成后的逻辑
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);

signals:
    // 通用 HTTP 请求完成信号
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);

    // 注册模块 HTTP 请求完成信号
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    // 重置模块 HTTP 请求完成信号
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);

    // 登录模块 HTTP 请求完成信号
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
