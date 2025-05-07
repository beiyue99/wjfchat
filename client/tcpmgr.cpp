#include "tcpmgr.h"
#include <QAbstractSocket>
#include "usermgr.h"
#include <QCoreApplication>
TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    // 连接成功后启动心跳
    connect(&_socket, &QTcpSocket::connected, this, [this](){
        _hbTimer.start(HB_INTERVAL);                  // ★ 新增
    });

    // 断开时停止心跳
    connect(&_socket, &QTcpSocket::disconnected, this, [this](){
        qDebug() << "Disconnected from server.";
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

    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
    // 当有数据可读时，读取所有数据
    // 读取所有数据并追加到缓冲区
    _buffer.append(_socket.readAll());

    QDataStream stream(&_buffer, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_5_0);

    forever {
         //先解析头部
        if(!_b_recv_pending){
            // 检查缓冲区中的数据是否足够解析出一个消息头（消息ID + 消息长度）
            if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
                return; // 数据不够，等待更多数据
            }

            // 预读取消息ID和消息长度，但不从缓冲区中移除
            stream >> _message_id >> _message_len;

            //将buffer 中的前四个字节移除
            _buffer = _buffer.mid(sizeof(quint16) * 2);
            // qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            // 消息包含好友信息，本身信息相关
        }

         //buffer剩余长读是否满足消息体长度，不满足则退出继续等待接受
        if(_buffer.size() < _message_len){
             _b_recv_pending = true;
             return;
        }

        _b_recv_pending = false;
        // 读取消息体
        QByteArray messageBody = _buffer.mid(0, _message_len);
//        qDebug() << "receive body msg is " << messageBody ;

        _buffer = _buffer.mid(_message_len);
        handleMsg(ReqId(_message_id),_message_len, messageBody);
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
        QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
        //注册消息
        initHandlers();
}

TcpMgr::~TcpMgr(){
    _hbTimer.stop();                     // 防止对象销毁后仍有定时器事件
    _socket.abort();
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

}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    _hbTimer.start(HB_INTERVAL);      // ★ 新增  收到包即复位
   auto find_iter =  _handlers.find(id);
   if(find_iter == _handlers.end()){
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

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;

    // 计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(dataBytes.length());

    // 创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    // 写入ID和长度
    out << id << len;

    // 添加字符串数据
    block.append(dataBytes);

    // 发送数据
    _socket.write(block);
//    qDebug() << "tcp mgr send byte data is " << block ;   //发送token 消息id，uid，长度等
}


