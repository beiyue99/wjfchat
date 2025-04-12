#ifndef CONTACTUSERLIST_H
#define CONTACTUSERLIST_H

#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QDebug>
#include <memory>
#include "userdata.h"

class ConUserItem;  // 前向声明：联系人项组件

class ContactUserList : public QListWidget
{
    Q_OBJECT
public:
    // 构造函数
    ContactUserList(QWidget* parent = nullptr);

    // 控制是否显示红点提示（如：有新好友申请）
    void ShowRedPoint(bool bshow = true);

protected:
    // 事件过滤器：监听滚轮事件，判断是否滚动到底部，加载更多联系人
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // 添加联系人列表中的所有联系人（内部构造使用）
    void addContactUserList();

public slots:
    // 处理点击联系人项事件
    void slot_item_clicked(QListWidgetItem *item);

    // 处理添加好友请求（好友申请的通知项）
    void slot_add_auth_firend(std::shared_ptr<AuthInfo>);

    // 处理好友申请响应
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);

signals:
    // 请求加载联系人（触发信号）
    void sig_loading_contact_user();

    // 切换到“添加好友”页面
    void sig_switch_apply_friend_page();

    // 切换到“好友信息”页面，参数为选中好友的信息
    void sig_switch_friend_info_page(std::shared_ptr<UserInfo> user_info);

private:
    bool _load_pending;                     // 是否正在加载联系人（防止重复触发）
    ConUserItem* _add_friend_item;         // “添加好友”按钮项
    QListWidgetItem* _groupitem;           // 分组标题项（如：我的好友）
};

#endif // CONTACTUSERLIST_H
