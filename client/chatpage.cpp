
#include "chatpage.h"
#include "ui_chatpage.h"
#include <QStyleOption>
#include <QPainter>
#include "chatitembase.h"
#include "textbubble.h"
#include "picturebubble.h"
#include "recvfilebubble.h"
#include "applyfrienditem.h"
#include "usermgr.h"
#include <QJsonArray>
#include <QJsonObject>
#include "tcpmgr.h"
#include <QUuid>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>


QMap<QString, RecvCtxPtr> _recvMap;    // ✅ 真正定义一次（不要再 extern）


ChatPage::ChatPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatPage)
{
    ui->setupUi(this);

    /* 把 file_lb 点击信号连到槽 */
    connect(ui->file_lb, &ClickedLabel::clicked,
            this,       &ChatPage::on_file_lb_clicked);
    /* 把本页面的 send_file 转给 TcpMgr，跨线程队列 */
    connect(this, &ChatPage::sig_send_file,
            TcpMgr::Inst(),
            &TcpMgr::slot_send_file,
            Qt::QueuedConnection);

    connect(TcpMgr::Inst(), &TcpMgr::sig_file_meta_rsp,
            this, &ChatPage::slot_file_meta_rsp);



    connect(TcpMgr::Inst(), &TcpMgr::sig_file_progress,
            this, [this](const QString& fid, qint64 bytes){
                if (auto bub = _fileBubbleMap.value(fid, nullptr))
                    bub->slot_set_progress(bytes);
            }, Qt::QueuedConnection);          // 跨线程，确保进 UI 线程


    // 初始化按钮状态（正常、悬停、按下）
//    ui->receive_btn->SetState("normal","hover","press");
    ui->send_btn->SetState("normal","hover","press");

    // 初始化图标状态（正常、悬停、按下）
    ui->emo_lb->SetState("normal","hover","press","normal","hover","press");
    ui->file_lb->SetState("normal","hover","press","normal","hover","press");


    connect(ui->chatEdit, &MessageTextEdit::send, this, &ChatPage::on_send_btn_clicked);


}

ChatPage::~ChatPage()
{
    delete ui;
}






void ChatPage::on_file_lb_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择发送文件");
    if (path.isEmpty() || !_user_info) return;

    QFileInfo fi(path);
    QString fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    _lastSendPath   = path;
    _lastSendFileId = fileId;

    /* ① UI 先插气泡 */
    ChatItemBase* chatItem = new ChatItemBase(ChatRole::Self);
    chatItem->setUserName(UserMgr::GetInstance()->GetUserInfo()->_name);
    chatItem->setUserIcon(QPixmap(UserMgr::GetInstance()->GetUserInfo()->_icon));

    auto bubble = new FileBubble(fi.fileName(), fi.size());
    _fileBubbleMap[fileId] = bubble;
    chatItem->setWidget(bubble);
    ui->chat_data_list->appendChatItem(chatItem);

    /* ② 点击“发送”按钮时，再发 1031 元数据 */
    connect(bubble, &FileBubble::sig_send_clicked, this, [=](){
        QJsonObject meta{
            {"file_id", fileId},
            {"fromuid", g_selfUid},
            {"touid"  , _user_info->_uid},
            {"size"   , fi.size()},
            {"fname"  , fi.fileName()}
        };
        QByteArray body = QJsonDocument(meta).toJson(QJsonDocument::Compact);
        emit TcpMgr::Inst()->sig_send_data(ID_FILE_META_REQ, body);
    });
}




void ChatPage::slot_file_meta_rsp(const QString& fileId,
                                  qint64 recvSize,
                                  int /*unused*/)
{
    if (fileId != _lastSendFileId) return;
    if (recvSize != -1)           return;   // 只在 “对方接受” 时触发

    /* 开始真正发分片 */
    QtConcurrent::run([=]{
        emit sig_send_file(_lastSendPath, fileId, 0, _user_info->_uid);
    });
}







// 设置聊天对象的信息，并将聊天记录加载出来
void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    /* 如果还是同一个人，就什么都别动 —— 保留现有气泡 */    // 改  增一句
    if (_user_info && _user_info->_uid == user_info->_uid)
        return;

    _user_info = user_info;
    ui->title_lb->setText(_user_info->_name);
    ui->chat_data_list->removeAllItem();
    for(auto & msg : user_info->_chat_msgs){
        AppendChatMsg(msg);
    }
}

// 将一条聊天消息追加到聊天窗口中
void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;

    if (msg->_from_uid == self_info->_uid) {
        // 自己发送的消息
        role = ChatRole::Self;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));
        QWidget* pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
    else {
        // 对方发送的消息
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if (friend_info == nullptr) {
            return;
        }
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));
        QWidget* pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
}

/* ====================================================================
 * 接收文件元数据 —— 在对话框插入“对方发来的文件气泡”
 * ===================================================================*/
void ChatPage::recvFileMeta(const QString& fid,
                            int            fromUid,
                            qint64         fileSize,
                            const QString& fname)
{
    /* ---------- ① UI 插气泡 ---------- */
    ChatItemBase *item = new ChatItemBase(ChatRole::Other);
    if (auto fi = UserMgr::GetInstance()->GetFriendById(fromUid)) {
        item->setUserName(fi->_name);
        item->setUserIcon(QPixmap(fi->_icon));
    }
    auto bubble = new RecvFileBubble(fname, fileSize, this);
    item->setWidget(bubble);
    ui->chat_data_list->appendChatItem(item);

    /* ---------- ② 构造 ctx ---------- */
    RecvCtxPtr ctx(new RecvCtx);
    ctx->total  = fileSize;
    ctx->bubble = bubble;
    ctx->tmp    = QSharedPointer<QFile>::create("./recv/" + fid + ".tmp");
    ctx->name   = fname;
    _recvMap.insert(fid, ctx);

    /* ---------- ③ 点击 接受 ---------- */
    connect(bubble, &RecvFileBubble::sig_accept, this, [=]{
        auto c = _recvMap.value(fid);
        if (!c || c->accepted) return;

        QDir().mkpath("./recv");
        if (!c->tmp->open(QIODevice::WriteOnly | QIODevice::Append)) {
            QMessageBox::warning(this, u8"保存失败", u8"无法打开临时文件");  return;
        }
        c->accepted = true;
        bubble->SetFileStatus(RecvFileBubble::STATUS_TRANSFERRING);

        /* ⇢ 告诉服务器：我已接受 —— 1032 */
        QJsonObject ack{
            {"file_id"  , fid},
            {"fromuid"  , fromUid},   // 原文件发送者
            {"touid"    , g_selfUid}, // 自己
            {"recv_size", -1}         // -1 = 已接受
        };
        emit TcpMgr::Inst()->sig_send_data(
                 ID_FILE_META_RSP,
                 QJsonDocument(ack).toJson(QJsonDocument::Compact));
    });

    /* ---------- ④ 点击 拒绝 ---------- */
    connect(bubble, &RecvFileBubble::sig_reject, this, [=]{
        bubble->SetFileStatus(RecvFileBubble::STATUS_REJECTED);
        bubble->slot_set_progress(0);
        _recvMap.remove(fid);

        /* ⇢ 告诉服务器：我拒绝 —— 1032，同样复用 */
        QJsonObject rej{
            {"file_id"  , fid},
            {"fromuid"  , fromUid},
            {"touid"    , g_selfUid},
            {"recv_size", -2}         // -2 = 拒绝
        };
        emit TcpMgr::Inst()->sig_send_data(
                 ID_FILE_META_RSP,
                 QJsonDocument(rej).toJson(QJsonDocument::Compact));
    });
}



// 支持样式的背景绘制
void ChatPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

// 发送按钮点击时触发，负责提取输入框内容，构造消息并发送
void ChatPage::on_send_btn_clicked()
{
    if (_user_info == nullptr) {
        qDebug() << "friend_info is empty";
        return;
    }

    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Self;

    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    QJsonObject textObj;
    QJsonArray textArray;
    int txt_size = 0;

    for(int i=0; i<msgList.size(); ++i)
    {
        // 跳过超长内容
        if(msgList[i].content.length() > 1024){
            continue;
        }

        QString type = msgList[i].msgFlag;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(user_info->_name);
        pChatItem->setUserIcon(QPixmap(user_info->_icon));
        QWidget *pBubble = nullptr;

        if(type == "text")
        {
            // 生成唯一消息 ID
            QUuid uuid = QUuid::createUuid();
            QString uuidString = uuid.toString();

            // 创建文字气泡
            pBubble = new TextBubble(role, msgList[i].content);

            // 如果超出单条 JSON 限制，则立即发送之前的数据
            if(txt_size + msgList[i].content.length()> 1024){
                textObj["fromuid"] = user_info->_uid;
                textObj["touid"] = _user_info->_uid;
                textObj["text_array"] = textArray;
                QJsonDocument doc(textObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

                // 发消息给服务器
                emit TcpMgr::Inst()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);

                // 清空计数器和缓存
                txt_size = 0;
                textArray = QJsonArray();
                textObj = QJsonObject();
            }

            // 累计本条消息
            txt_size += msgList[i].content.length();

            // 构造 JSON 格式内容
            QJsonObject obj;
            obj["content"] = QString::fromUtf8(msgList[i].content.toUtf8());
            obj["msgid"] = uuidString;
            textArray.append(obj);

            // 发送本地 UI 信号（本地预览用）
            auto txt_msg = std::make_shared<TextChatData>(uuidString, obj["content"].toString(),
                user_info->_uid, _user_info->_uid);
            emit sig_append_send_chat_msg(txt_msg);
        }
        else if(type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }
        else if(type == "file")
        {
            // TODO: 文件发送暂未实现
        }

        if(pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }

    // 处理循环结束后剩下的未发送内容
    textObj["text_array"] = textArray;
    textObj["fromuid"] = user_info->_uid;
    textObj["touid"] = _user_info->_uid;
    QJsonDocument doc(textObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    emit TcpMgr::Inst()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
}







void ChatPage::recvFileData(const QString& fid, qint64 offset, const QByteArray& raw)
{
    auto ctx = _recvMap.value(fid);
    if (!ctx || !ctx->accepted) return;
    /* ⬇ 避免 bubble 指针已失效导致崩溃 */
    if (!ctx->bubble)               // 页面被清空 / 重新创建
        return;                     // 先简单忽略（至少不卡死）

//if (ctx->tmp->pos() != offset) return;      // 验证偏移 增
    // ✅ 检查是否点击“接受”并成功打开文件
    if (!ctx)                         qDebug() << "ctx null";
    else if (!ctx->accepted)          qDebug() << "not accepted yet";
    else if (!ctx->tmp)               qDebug() << "tmp null";
    else if (!ctx->tmp->isOpen())     qDebug() << "file not open";


    qint64 pos = ctx->tmp->pos();
    if (pos != offset) {
        qWarning() << "[recv] offset mismatch, 当前=" << pos << " 期望=" << offset;
        return;
    }

    ctx->tmp->write(raw);             // ✅ 写入文件
    ctx->written += raw.size();

    ctx->bubble->slot_set_progress(ctx->written);  // ✅ 更新 UI 进度
}









// 清空聊天内容显示区
void ChatPage::clearItems()
{
    ui->chat_data_list->removeAllItem();
}
