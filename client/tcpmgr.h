#ifndef TCPMGR_H
#define TCPMGR_H

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include <QJsonArray>
#include "userdata.h"

/**
 * @brief TcpMgr 类，管理 TCP 连接及数据收发
 * 继承自 QObject, Singleton<TcpMgr>，并使用 std::enable_shared_from_this<TcpMgr>
 */
class TcpMgr : public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr(); // 析构函数

private:
    friend class Singleton<TcpMgr>; // 允许 Singleton 访问私有构造函数
    TcpMgr(); // 构造函数

    /**
     * @brief 初始化消息处理函数映射表
     */
    void initHandlers();

    /**
     * @brief 处理接收到的消息
     * @param id 请求 ID
     * @param len 数据长度
     * @param data 消息数据
     */
    void handleMsg(ReqId id, int len, QByteArray data);

    QTcpSocket _socket;  ///< TCP 套接字对象
    QString _host;       ///< 服务器地址
    uint16_t _port;      ///< 服务器端口号
    QByteArray _buffer;  ///< 接收数据缓冲区
    bool _b_recv_pending; ///< 是否有未处理的接收数据
    quint16 _message_id; ///< 当前消息 ID
    quint16 _message_len; ///< 当前消息长度

    /**
     * @brief 消息处理映射表，根据请求 ID 绑定对应的处理函数
     */
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

public slots:
    /**
     * @brief 连接到服务器
     * @param serverInfo 服务器信息
     */
    void slot_tcp_connect(ServerInfo serverInfo);

    /**
     * @brief 发送数据到服务器
     * @param reqId 请求 ID
     * @param data 发送的数据
     */
    void slot_send_data(ReqId reqId, QByteArray data);

signals:
    void sig_con_success(bool bsuccess); ///< 连接成功信号
    void sig_send_data(ReqId reqId, QByteArray data); ///< 发送数据信号
    void sig_swich_chatdlg(); ///< 切换聊天窗口信号
    void sig_login_failed(int); ///< 登录失败信号
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
};

#endif // TCPMGR_H
