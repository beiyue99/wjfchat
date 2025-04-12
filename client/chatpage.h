#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include "userdata.h"
#include <QMap>

namespace Ui {
class ChatPage;
}

// 单个聊天页面，包含用户信息展示和消息收发界面
class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr); // 构造函数，初始化 UI
    ~ChatPage(); // 析构函数，释放资源

    void SetUserInfo(std::shared_ptr<UserInfo> user_info); // 设置当前聊天的用户信息
    void AppendChatMsg(std::shared_ptr<TextChatData> msg); // 将消息内容追加到聊天窗口中

protected:
    void paintEvent(QPaintEvent *event); // 重绘事件，用于绘制背景或边框等

private slots:
    void on_send_btn_clicked(); // 点击发送按钮后的槽函数
    void on_receive_btn_clicked(); // 模拟接收消息的槽函数（调试或演示用）

private:
    void clearItems(); // 清除聊天记录的 UI 项

    Ui::ChatPage *ui; // UI 对象指针
    std::shared_ptr<UserInfo> _user_info; // 当前聊天的用户信息
    QMap<QString, QWidget*>  _bubble_map; // 聊天气泡映射（消息唯一标识 -> 对应气泡组件）

signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData> msg); // 发送消息时发出的信号，用于通知外部处理逻辑
};

#endif // CHATPAGE_H
