#include "tcpmgr.h"
#include <QAbstractSocket>
#include "usermgr.h"
#include <QCoreApplication>
#include <QThread>
#include <QtEndian>
#include "chatpage.h"


TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{

    qRegisterMetaType<ReqId>("ReqId");
    // 连接成功后启动心跳
    connect(&_socket, &QTcpSocket::connected, this, [this](){
        _hbTimer.start(HB_INTERVAL);                  // ★ 新增
    });

    // 断开时停止心跳
    connect(&_socket, &QTcpSocket::disconnected, this, [this](){
        qWarning() << "[SOCK] disconnected() emitted";
        _hbTimer.stop();                              // ★ 新增

        if (QCoreApplication::closingDown()) return;        // ★ 应用准备退出，直接返回

        // ★ 新增：断开 3 秒后自动重连
        QTimer::singleShot(3000, this, [this](){
            if (_socket.state() == QAbstractSocket::UnconnectedState) {
                qDebug() << "Reconnecting to" << _host << ":" << _port;
                _socket.connectToHost(_host, _port);
            }
        });
    });


    connect(&_socket, &QTcpSocket::bytesWritten,
            this,  &TcpMgr::slot_continue_write);   // ★ 新槽

    // 发送心跳
    connect(&_hbTimer, &QTimer::timeout, this, [this](){   // ★ 新增
        if (_socket.state() == QAbstractSocket::ConnectedState) {
            QByteArray empty;                          // 无负载
            emit sig_send_data(ID_HEARTBEAT_REQ, empty);
        }
    });


    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server!";
        // 连接建立后发送消息
        emit sig_con_success(true);
    });

    connect(&_socket, &QTcpSocket::readyRead, this, [this]()
    {
        _buffer.append(_socket.readAll());

        while (true) {
            if (_buffer.size() < 4) break;

            uchar head[4];
            memcpy(head, _buffer.constData(), 4);

            quint16 msgId  = qFromBigEndian<quint16>(head);
            quint16 msgLen = qFromBigEndian<quint16>(head + 2);

            if (_buffer.size() < 4 + msgLen) break;

            _buffer.remove(0,4);
            QByteArray body = _buffer.left(msgLen);
            _buffer.remove(0,msgLen);

            handleMsg(static_cast<ReqId>(msgId), msgLen, body);
        }

    });


//5.15 之后版本
//    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
//        Q_UNUSED(socketError)
//        qDebug() << "Error:" << _socket.errorString();
//    });

    // 处理错误（适用于Qt 5.15之前的版本）
     QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
                            [&](QTcpSocket::SocketError socketError) {
               qDebug() << "Error:" << _socket.errorString() ;
               switch (socketError) {
                   case QTcpSocket::ConnectionRefusedError:
                       qDebug() << "Connection Refused!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::RemoteHostClosedError:
                       qDebug() << "Remote Host Closed Connection!";
                       break;
                   case QTcpSocket::HostNotFoundError:
                       qDebug() << "Host Not Found!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::SocketTimeoutError:
                       qDebug() << "Connection Timeout!";
                       emit sig_con_success(false);
                       break;
                   case QTcpSocket::NetworkError:
                       qDebug() << "Network Error!";
                       break;
                   default:
                       qDebug() << "Other Error!";
                       break;
               }
         });
        //连接发送信号用来发送数据
//        connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
        connect(this,&TcpMgr::sig_send_data,this,&TcpMgr::slot_send_data, Qt::QueuedConnection);




        /* 把后台线程抛上来的原始包写进 socket —— Qt::QueuedConnection 保证跨线程安全 */
        connect(this, &TcpMgr::sig_raw_packet,
                this, [this](ReqId id, const QByteArray& body)
        {
            QByteArray pkt;
            QDataStream out(&pkt, QIODevice::WriteOnly);
            out.setByteOrder(QDataStream::BigEndian);
            out << quint16(id) << quint16(body.size());
            pkt.append(body);
            _socket.write(pkt);
        }, Qt::QueuedConnection);
        //注册消息
        initHandlers();
        // ① 连接状态变化
        connect(&_socket,&QTcpSocket::stateChanged, this,
                [](QTcpSocket::SocketState st){
            qDebug().noquote() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                               << "[SOCK] state =" << st;          // 0-6
        });

        // ② Qt 级错误 (包括 10054)
        connect(&_socket,
                QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this,
                [](QAbstractSocket::SocketError e){
                    qDebug().noquote()
                            << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                            << "[SOCK] error =" << e;
        });


        // ③ 每次真正写入内核缓冲的字节数
        connect(&_socket,&QTcpSocket::bytesWritten, this,
                [](qint64 n){
            qDebug().noquote() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                               << "[SOCK] bytesWritten" << n;
        });

        connect(this, &TcpMgr::sig_raw_packet,
                this, &TcpMgr::slot_send_data,
                Qt::QueuedConnection);     // 一定要 Queued



        // TcpMgr 构造函数末尾加一次即可
        connect(&_socket, &QTcpSocket::readyRead, this,
                [](){ qDebug() << "[SOCK] readyRead"; });

        connect(&_socket, &QTcpSocket::bytesWritten, this,
                [](qint64 n){ qDebug() << "[SOCK] bytesWritten" << n; });


}

TcpMgr::~TcpMgr(){
    _hbTimer.stop();                     // 防止对象销毁后仍有定时器事件
//    _socket.abort();
}

TcpMgr* TcpMgr::Inst()
{
    /* 进程生命周期只 new 一次，Qt 自动在主线程析构所有 QObject，
       不需要手动 delete；泄露由 OS 回收。*/
    static TcpMgr* s = new TcpMgr;
    return s;
}


/* 生成完整包并压入队列，由主线程负责真正写 */
void TcpMgr::enqueuePacket(ReqId id, const QByteArray& body)
{
    QByteArray pkt;
    QDataStream out(&pkt, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << quint16(id) << quint16(body.size());
    pkt.append(body);

    {
        QMutexLocker lk(&_queueMtx);
        _sendQueue.enqueue(pkt);
    }
    QMetaObject::invokeMethod(this, "slot_continue_write",
                              Qt::QueuedConnection);
}


void TcpMgr::slot_continue_write()
{
    QMutexLocker lk(&_queueMtx);
    if (_writing) return;                 // 还有一次写尚未结束

    if (_sendQueue.isEmpty()) return;

    QByteArray pkt = _sendQueue.dequeue();
    _writing = true;                      // 标记正在写
    lk.unlock();

    _socket.write(pkt);                   // 写完会再次触发 bytesWritten
    _socket.flush();

    lk.relock();
    _writing = false;                     // 本次写完
}


/* ========== ★ 新增：分片发送线程槽 ========= */
void TcpMgr::slot_send_file(const QString& filePath,
                            const QString& fileId,
                            qint64  resumeOff,
                            int32_t toUid)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    f.seek(resumeOff);

    constexpr qint64 CHUNK = 900;
    qint64 off = resumeOff;

    while (!f.atEnd())
    {
        QByteArray buf = f.read(CHUNK);

        QJsonObject j{
            {"file_id", fileId},
            {"offset" , off},
            {"touid"  , toUid},
            {"data"   , QString(buf.toBase64())}
        };
        off += buf.size();

        enqueuePacket(ID_FILE_DATA_REQ,
                      QJsonDocument(j).toJson(QJsonDocument::Compact));

        emit sig_file_progress(fileId, off);
    }

    QJsonObject fin{{"file_id", fileId}, {"touid", toUid}};
    enqueuePacket(ID_FILE_FINISH_REQ,
                  QJsonDocument(fin).toJson(QJsonDocument::Compact));

    // ② 本地 UI 也要知道已经结束 —— 关键就在这行
    emit sig_file_progress(fileId, -1);      // <—— 新增
}





void TcpMgr::initHandlers()
{
//    auto self = shared_from_this();   类还没有构造完，不能这样写
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);  // 忽略len参数，未使用

        // 将QByteArray数据转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查QJsonDocument是否创建成功
        if(jsonDoc.isNull()){
            qDebug() << "Failed to create QJsonDocument.";  // 如果创建失败，输出错误信息
            return;  // 直接返回，处理失败
        }

        // 获取QJsonDocument中的QJsonObject
        QJsonObject jsonObj = jsonDoc.object();

        // 检查返回的JSON对象中是否包含"error"字段，若未包含则认为解析失败
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;  // 定义错误码为JSON解析错误
            qDebug() << "Login Failed, err is Json Parse Err" << err ;  // 输出解析错误信息
            emit sig_login_failed(err);  // 发送登录失败的信号
            return;  // 返回，停止处理
        }

        // 获取返回的错误码
        int err = jsonObj["error"].toInt();
        // 如果错误码不等于成功状态，表示登录失败
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;  // 输出失败的错误码
            emit sig_login_failed(err);  // 发送登录失败的信号
            return;  // 返回，停止处理
        }

        // 提取登录成功时的用户信息
        auto uid = jsonObj["uid"].toInt();  // 用户ID
        auto name = jsonObj["name"].toString();  // 用户名
        auto icon = jsonObj["icon"].toString();  // 用户头像

        // 创建用户信息对象
        auto user_info = std::make_shared<UserInfo>(uid, name, icon );

        // 将用户信息存储到UserMgr中
        UserMgr::GetInstance()->SetUserInfo(user_info);

        // 设置用户的token
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        // 如果返回的JSON中包含"apply_list"字段，添加到用户申请列表
        if(jsonObj.contains("apply_list")){
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }

        // 如果返回的JSON中包含"friend_list"字段，添加到用户好友列表
        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        // 登录成功，切换到聊天界面
        emit sig_swich_chatdlg();
    });



    _handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);  // 忽略len参数，未使用

        // 将QByteArray数据转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查QJsonDocument是否成功创建
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";  // 如果创建失败，输出错误信息
            return;  // 返回，停止处理
        }

        // 获取QJsonDocument中的QJsonObject
        QJsonObject jsonObj = jsonDoc.object();

        // 检查返回的JSON对象中是否包含"error"字段，若未包含则认为解析失败
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;  // 定义错误码为JSON解析错误
            qDebug() << "search user Failed, err is Json Parse Err" << err;  // 输出解析错误信息

            emit sig_user_search(nullptr);  // 发送用户搜索失败的信号
            return;  // 返回，停止处理
        }

        // 获取返回的错误码
        int err = jsonObj["error"].toInt();
        // 如果错误码不等于成功状态，表示搜索失败
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "search user Failed, err is " << err;  // 输出失败的错误码
            emit sig_user_search(nullptr);  // 发送用户搜索失败的信号
            return;  // 返回，停止处理
        }

        // 提取搜索到的用户信息
        auto search_info = std::make_shared<SearchInfo>(
            jsonObj["uid"].toInt(),         // 用户ID
            jsonObj["name"].toString(),     // 用户名
            jsonObj["icon"].toString()      // 头像
        );
        qDebug() << "tcpMgr icon is :" << jsonObj["icon"].toString();
        qDebug() << "tcpMgr name is :" << jsonObj["name"].toString();
        // 发出用户搜索成功的信号，传递用户信息
        emit sig_user_search(search_info);
    });


    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);  // 忽略len参数，未使用

        // 将QByteArray数据转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查QJsonDocument是否成功创建
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";  // 如果创建失败，输出错误信息
            return;  // 返回，停止处理
        }

        // 获取QJsonDocument中的QJsonObject
        QJsonObject jsonObj = jsonDoc.object();

        // 检查返回的JSON对象中是否包含"error"字段，若未包含则认为解析失败
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;  // 定义错误码为JSON解析错误
            qDebug() << "add friend req Failed, err is Json Parse Err" << err;  // 输出解析错误信息

            emit sig_user_search(nullptr);  // 发送用户搜索失败的信号
            return;  // 返回，停止处理
        }

        // 获取返回的错误码
        int err = jsonObj["error"].toInt();
        // 如果错误码不等于成功状态，表示添加好友请求失败
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "add friend req Failed, err is " << err;  // 输出失败的错误码
            emit sig_user_search(nullptr);  // 发送用户搜索失败的信号
            return;  // 返回，停止处理
        }

        // 提取发起添加好友请求的用户信息
        int from_uid = jsonObj["applyuid"].toInt();  // 提取申请人用户ID
        QString name = jsonObj["name"].toString();   // 提取申请人姓名
        QString icon = jsonObj["icon"].toString();   // 提取申请人头像

        // 创建一个AddFriendApply对象，封装申请人的信息
        auto apply_info = std::make_shared<AddFriendApply>(
            from_uid, name, icon
        );

        // 发出添加好友请求的信号，并传递申请人的信息
        emit sig_friend_apply(apply_info);
    });


    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString icon = jsonObj["icon"].toString();

        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,icon );

        emit sig_add_auth_friend(auth_info);
        });

    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Send Friend apply Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Send Friend apply Failed, err is " << err;
            return;
        }

         qDebug() << "Send Friend apply Success " ;
      });


    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        auto name = jsonObj["name"].toString();
        auto icon = jsonObj["icon"].toString();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name,  icon);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success " ;
      });


    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }
        qDebug() << "message has send succeed";
      });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Notify Success " ;
        auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg_ptr);
      });

    // 新增函数
    _handlers.insert(ID_HEARTBEAT_RSP, [this](ReqId, int, QByteArray){
        _hbTimer.start(HB_INTERVAL);      // ★ 服务端回包也复位
    });

    /* ① 文件元数据 1031  FileMeta 通知 */
    _handlers[ID_FILE_META_REQ] =
        [this](ReqId, int, const QByteArray& body)
    {

        qDebug() << "[TCP] recv 1031 JSON =" << QString::fromUtf8(body);

        const QJsonObject o = QJsonDocument::fromJson(body).object();

        const QString fid   = o["file_id"].toString();

        //------------------ 只改下面这一行 ------------------
//        QString strFrom = o["fromuid"].toString();
//        bool ok = false;
//        int from = strFrom.toInt(&ok);
//        if (!ok) {
//            qWarning() << "[TCP] 无法解析 fromuid，原始值为:" << o["fromuid"];
//            return;
//        }
            int from = o.value("fromuid").toVariant().toInt();   // ← 关键一行

            qDebug() << "fromid is xxxxxxxxxxxx" << from;

           const qint64  size  = o["size"].toVariant().toLongLong();
           const QString fname = o["fname"].toString();

        qDebug() << "[TCP] 1031 for fid" << fid << "from" << from;

        emit sig_in_file_meta(fid, from, size, fname);

        if (!UserMgr::GetInstance()->GetFriendById(from)) {
            qWarning() << "⚠️ 无法找到发送者 uid=" << from << "，file_id=" << fid;
        }

    };


    /* ② 文件分片 1033  FileChunk*/
    _handlers[ID_FILE_DATA_REQ] =
        [this](ReqId, int, QByteArray body)
    {
        QJsonObject o = QJsonDocument::fromJson(body).object();
        emit sig_in_file_chunk(                       //  ← 还是老信号
                o["file_id"].toString(),
                o["offset"].toVariant().toLongLong(),
                QByteArray::fromBase64(
                        o["data"].toString().toUtf8()));
    };




    /* ③ 文件完成 1035  FileFinish 通知 */
    // 添加在 1041 响应处理里
    _handlers[ID_FILE_FINISH_REQ] =
        [this](ReqId, int, const QByteArray& body)
    {
        const QJsonObject o = QJsonDocument::fromJson(body).object();
        const QString fid   = o["file_id"].toString();
        qDebug() << "[TCP] 1041 文件传输完成:" << fid;
        // 通知所有 UI 层
        emit sig_in_file_finish(fid);             // ← ★ 就是这一句
        if (_recvMap.contains(fid)) {
            auto ctx = _recvMap.value(fid);
            if (ctx && ctx->bubble) {
                ctx->bubble->SetFileStatus(RecvFileBubble::STATUS_FINISHED);
            }
        }
    };


    /* TcpMgr —— 1032 回包 handler */
    _handlers[ID_FILE_META_RSP] = [this](ReqId, int, QByteArray data)
    {
        auto obj = QJsonDocument::fromJson(data).object();
        QString  fileId   = obj["file_id"].toString();
        qint64   recvSize = obj["recv_size"].toVariant().toLongLong();
        emit sig_file_meta_rsp(fileId, recvSize, -1);   // 无 touid
    };

    _handlers[ID_FILE_FINISH_RSP] =
        [](ReqId, int, QByteArray){
            // nothing to do, just avoid ASSERT failure
        };



    _handlers[ID_FILE_DATA_RSP] = [](ReqId, int, QByteArray){};


    _handlers[ID_FILE_ACCEPT_RSP] =
        [this](ReqId, int, const QByteArray& body){
            QJsonObject o = QJsonDocument::fromJson(body).object();
            QString fid  = o["file_id"].toString();
            int action   = o["action"].toInt();
            emit sig_file_accept(fid, action);      // 自定义信号
        };



}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    qDebug().noquote()
        << "[DISPATCH] id=" << id
        << " len="        << len;
    _hbTimer.start(HB_INTERVAL);      // ★ 新增  收到包即复位
   auto find_iter =  _handlers.find(id);
   if(find_iter == _handlers.end()){
       qWarning().noquote() << "[TCP] unknown id" << int(id)
                            << "len" << len;
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
   }

   find_iter.value()(id,len,data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);
}

void TcpMgr::slot_send_data(ReqId id, QByteArray body)
{
    enqueuePacket(id, body);              // ← 只排队
}



