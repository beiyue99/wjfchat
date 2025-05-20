#ifndef TCPMGR_H
#define TCPMGR_H

#include <QTcpSocket>
//#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"
#include <QJsonArray>
#include <QTimer>
#include "filebubble.h"
#include <QElapsedTimer>
#include <QQueue>
#include <QMutex>


class TcpMgr : public QObject
{
    Q_OBJECT
public:
    // 析构函数，销毁对象时清理资源
   ~TcpMgr();

    static TcpMgr* Inst();
private:

    TcpMgr();                // 返回裸指针给所有模块

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


    QTimer _hbTimer;          // ★ 新增：心跳发送定时器
    static const int HB_INTERVAL = 30 * 1000; // ★ 30 s


    QQueue<QByteArray> _sendQueue;          // 主线程写 socket 用
    bool               _writing = false;    // 标记是否正在写
    QMutex             _queueMtx;           // 保护 _sendQueue

public:
    void enqueuePacket(ReqId id, const QByteArray& body);
public slots:
    // 用于连接服务器，接收服务器的地址和端口
    void slot_tcp_connect(ServerInfo);

    // 用于发送数据到服务器，传递请求 ID 和数据内容
    void slot_send_data(ReqId reqId, QByteArray data);

    /* ★ 新增：后台线程分片发送 */
    void slot_send_file(const QString& filePath,
                        const QString& fileId,
                        qint64  resumeOffset,
                        int32_t toUid);

    void slot_continue_write();
signals:
    //新增文件发送相关信号
    void sig_file_meta_rsp(const QString& fileId,
                           qint64 recvSize,
                           int   toUid);

    void sig_file_progress(const QString& fileId, qint64 bytes);

    void sig_in_file_meta(QString fileId,int fromUid,qint64 size,QString fname);
    void sig_in_file_chunk(QString fileId,qint64 offset,QByteArray data);
    void sig_in_file_finish(QString fileId);
    /* ★ 新增：发送端收到 1042 后抛这个信号给 UI */
    void sig_file_accept(const QString& fileId, int action);




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

    void sig_raw_packet(ReqId id, QByteArray body);   // 线程安全写 socket

};

#endif // TCPMGR_H
