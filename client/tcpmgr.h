#ifndef TCPMGR_H
#define TCPMGR_H

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"
#include <QJsonArray>

class TcpMgr: public QObject, public Singleton<TcpMgr>,
        public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    // 析构函数，销毁对象时清理资源
   ~TcpMgr();

private:
    // 友元类：允许 Singleton 类访问 TcpMgr 的私有构造函数
    friend class Singleton<TcpMgr>;

    // 私有构造函数：防止外部直接创建实例，确保单例模式
    TcpMgr();

    // 初始化消息处理函数（handlers）
    void initHandlers();

    // 处理接收到的消息
    void handleMsg(ReqId id, int len, QByteArray data);

    // QTcpSocket 对象，负责实际的 TCP 网络通信
    QTcpSocket _socket;

    // 服务器主机地址
    QString _host;

    // 服务器端口
    uint16_t _port;

    // 接收到的数据缓冲区
    QByteArray _buffer;

    // 标记是否存在待接收的数据
    bool _b_recv_pending;

    // 消息的 ID 和长度
    quint16 _message_id;
    quint16 _message_len;

    // 消息处理函数的映射表：根据 ReqId 查找对应的处理函数
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

public slots:
    // 用于连接服务器，接收服务器的地址和端口
    void slot_tcp_connect(ServerInfo);

    // 用于发送数据到服务器，传递请求 ID 和数据内容
    void slot_send_data(ReqId reqId, QByteArray data);

signals:
    // 连接成功时发出的信号，参数表示是否成功
    void sig_con_success(bool bsuccess);

    // 发送数据信号
    void sig_send_data(ReqId reqId, QByteArray data);

    // 切换聊天窗口的信号
    void sig_swich_chatdlg();

    // 加载好友申请列表的信号
    void sig_load_apply_list(QJsonArray json_array);

    // 登录失败时发出的信号，传递失败的错误代码
    void sig_login_failed(int);

    // 搜索用户的信号
    void sig_user_search(std::shared_ptr<SearchInfo>);

    // 好友申请信号，包含好友申请的详细信息
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);

    // 处理好友认证的信号，将处理结果发给服务器
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);

    // 好友认证响应信号,收到后隐藏add按钮
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);

    // 文本聊天消息的信号，传递聊天消息内容
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
};

#endif // TCPMGR_H
