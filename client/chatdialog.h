#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QList>
#include "statelabel.h"
#include "global.h"
#include "statewidget.h"
#include <memory>
#include "userdata.h"
#include <QListWidgetItem>

#include "applyfriendpage.h"



namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数
    explicit ChatDialog(QWidget *parent = nullptr);
    // 析构函数
    ~ChatDialog();


protected:
    // 事件过滤器，用于处理各种事件
    bool eventFilter(QObject *watched, QEvent *event) override ;

    // 处理全局鼠标按下事件
    void handleGlobalMousePress(QMouseEvent *event);
    // 关闭查找对话框
    void CloseFindDlg();
    // 更新聊天消息
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);

private:
    // 添加状态标签组
    void AddLBGroup(StateWidget* lb);
    // 添加聊天用户列表
    void addChatUserList();
    // 加载更多聊天用户
    void loadMoreChatUser();
    // 清除标签状态
    void ClearLabelState(StateWidget* lb);
    // 加载更多联系人
    void loadMoreConUser();
    // 设置选中的聊天项
    void SetSelectChatItem(int uid = 0);
    // 设置选中的聊天页面
    void SetSelectChatPage(int uid = 0);

    // UI 界面指针
    Ui::ChatDialog *ui;

    // 是否正在加载数据
    bool _b_loading;

    // 存储状态标签列表
    QList<StateWidget*> _lb_list;

    // 显示或隐藏搜索框
    void ShowSearch(bool bsearch = false);

    // 当前界面的显示模式
    ChatUIMode _mode;

    // 当前界面的状态
    ChatUIMode _state;

    // 上一个激活的控件
    QWidget* _last_widget;

    // 存储聊天项的映射
    QMap<int, QListWidgetItem*> _chat_items_added;

    // 当前选中的聊天用户 ID
    int _cur_chat_uid;

public slots:
    // 加载聊天用户的槽函数
    void slot_loading_chat_user();
    // 显示聊天侧边栏的槽函数
    void slot_side_chat();
    // 显示联系人侧边栏的槽函数
    void slot_side_contact();
    // 文字框内容改变时触发的槽函数
    void slot_text_changed(const QString &str);
    // 失去焦点时触发的槽函数
    void slot_focus_out();
    // 加载联系人用户的槽函数
    void slot_loading_contact_user();
    // 切换到好友申请页面的槽函数
    void slot_switch_apply_friend_page();
    // 显示好友信息页面的槽函数
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    // 显示或隐藏搜索页面的槽函数
    void slot_show_search(bool show);
    // 处理好友申请的槽函数
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    // 处理授权信息的槽函数
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    // 处理授权响应的槽函数
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    // 跳转到指定的聊天项
    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    // 从信息页面跳转到指定的聊天项
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> ui);
    // 点击聊天列表项时触发的槽函数
    void slot_item_clicked(QListWidgetItem *item);
    // 接收到新的文本聊天消息时触发的槽函数
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    // 向聊天界面追加发送的聊天消息
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
signals:
    void cancel_red();
private slots:
    // 私有槽函数
};

#endif // CHATDIALOG_H
