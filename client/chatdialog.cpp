#include "chatdialog.h"
#include "ui_chatdialog.h"
#include <QAction>
#include "chatuserwid.h"
#include <QDebug>
#include <vector>
#include <QRandomGenerator>
#include <QTimer>
#include "loadingdlg.h"
#include "global.h"
#include "chatitembase.h"
#include "textbubble.h"
#include "picturebubble.h"
#include "messagetextedit.h"
#include "chatuserlist.h"
#include "grouptipitem.h"
#include "invaliditem.h"
#include "conuseritem.h"
#include "lineitem.h"
#include "tcpmgr.h"
#include "usermgr.h"


ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog),
    _b_loading(false),
    _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),
    _last_widget(nullptr),
    _cur_chat_uid(0)
{
    ui->setupUi(this); // 初始化 UI 组件

    // 设置添加按钮的状态资源（normal、hover、press）
    ui->add_btn->SetState("normal", "hover", "press");
    ui->add_btn->setProperty("state", "normal");

    // 配置搜索框前置图标为搜索图标
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    // 配置搜索框后置清除按钮（初始为透明图标）
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    // 当搜索框内容变化时，控制清除按钮图标是否显示
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png"));
        }
    });

    // 清除按钮被点击时，清空搜索框内容并隐藏清除图标
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png"));
        ui->search_edit->clearFocus();
        ShowSearch(false); // 清除时不显示搜索页面
    });

    // 设置搜索框最大长度为15个字符
    ui->search_edit->SetMaxLength(15);

    // 连接聊天用户列表加载信号到槽函数
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);

    // 加载聊天用户列表 UI
    addChatUserList();

    // 设置侧边栏头像为当前用户头像
    QString head_icon = UserMgr::GetInstance()->GetIcon();
    QPixmap pixmap(head_icon);
    QPixmap scaledPixmap = pixmap.scaled(ui->side_head_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->side_head_lb->setPixmap(scaledPixmap);
    ui->side_head_lb->setScaledContents(true);

    // 设置侧边聊天按钮初始状态
    ui->side_chat_lb->setProperty("state", "normal");
    ui->side_chat_lb->SetState("normal", "hover", "pressed", "selected_normal", "selected_hover", "selected_pressed");
    ui->side_contact_lb->SetState("normal", "hover", "pressed", "selected_normal", "selected_hover", "selected_pressed");

    // 添加按钮进按钮组，控制选中状态互斥
    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);

    // 连接侧边栏按钮点击信号与槽函数
    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    // 搜索框内容改变时触发槽函数
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

    // 初始隐藏搜索页面
    ShowSearch(false);

    // 安装事件过滤器用于监听全局点击事件（实现点击空白处自动隐藏搜索框等功能）
    this->installEventFilter(this);

    // 初始选中聊天侧边按钮
    ui->side_chat_lb->SetSelected(true);

    // 选中聊天用户列表第一个项
    SetSelectChatItem();

    // 更新聊天界面为当前选中用户
    SetSelectChatPage();

    // 连接联系人页面用户加载信号
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user, this, &ChatDialog::slot_loading_contact_user);

    // 连接联系人页面点击“好友申请”按钮信号
    connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page, this, &ChatDialog::slot_switch_apply_friend_page);

    // 当好友申请页面点击清除搜索框按钮时触发的槽函数
    connect(ui->friend_apply_page, &ApplyFriendPage::sig_show_search, this, &ChatDialog::slot_show_search);

    // 设置搜索结果列表使用当前搜索框
    ui->search_list->SetSearchEdit(ui->search_edit);

    // 监听 TCP 层发来的好友申请消息，跳转到相应 UI
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_friend_apply, this, &ChatDialog::slot_apply_friend);

    // 监听 TCP 层发来的认证好友请求
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);

    // 监听服务器返回的好友认证结果
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);

    // 监听联系人页点击用户头像信号，跳转用户信息界面
    connect(ui->con_user_list, &ContactUserList::sig_switch_friend_info_page, this, &ChatDialog::slot_friend_info_page);

    // 设置初始中心页面为聊天界面
    ui->stackedWidget->setCurrentWidget(ui->chat_page);

    // 监听从搜索结果点击跳转到聊天窗口
    connect(ui->search_list, &SearchList::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item);

    // 监听从用户信息界面跳转到聊天窗口
    connect(ui->friend_info_page, &FriendInfoPage::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item_from_infopage);

    // 监听聊天列表点击事件
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_item_clicked);

    // 监听接收到对方发来的聊天消息通知
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_text_chat_msg, this, &ChatDialog::slot_text_chat_msg);

    // 监听当前用户发送完消息后的信号，将消息显示在聊天页面
    connect(ui->chat_page, &ChatPage::sig_append_send_chat_msg, this, &ChatDialog::slot_append_send_chat_msg);


    connect(this, &ChatDialog::cancel_red, ui->side_contact_lb, &StateWidget::slot_on_cancel_red);

}


ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::slot_item_clicked(QListWidgetItem *item)
{
    // 获取 QListWidgetItem 对应的自定义 widget（用户项）
    QWidget *widget = ui->chat_user_list->itemWidget(item);
    if(!widget){
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 将 widget 强制转换为基类 ListItemBase，用于统一管理
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 获取该项的类型：无效项、分组标题项、聊天用户项等
    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM
        || itemType == ListItemType::GROUP_TIP_ITEM){
        // 无效或仅用于分组显示的 item，忽略点击
        return;
    }

    // 如果是聊天用户项，切换到对应用户的聊天页面
    if(itemType == ListItemType::CHAT_USER_ITEM){
        // 转换为 ChatUserWid 类型，访问具体用户信息
        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        auto user_info = chat_wid->GetUserInfo();

        // 设置聊天页面的用户信息（右侧聊天区域更新）
        ui->chat_page->SetUserInfo(user_info);

        // 记录当前聊天的用户 UID
        _cur_chat_uid = user_info->_uid;
        return;
    }
}


void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    // 查找当前聊天用户列表中是否已经添加过该好友项（根据 UID）
    auto find_iter = _chat_items_added.find(msg->_from_uid);

    if(find_iter != _chat_items_added.end()){
        // 如果找到了，说明该好友已存在于聊天列表

        // 获取该好友在 chat_user_list 中对应的 widget
        QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
        auto chat_wid = qobject_cast<ChatUserWid*>(widget);
        if(!chat_wid){
            return; // 类型转换失败，跳过
        }

        // 更新该好友项显示的最后一条消息
        chat_wid->updateLastMsg(msg->_chat_msgs);

        // 如果刚好当前聊天窗口是这个好友，也更新右侧聊天页面
        UpdateChatMsg(msg->_chat_msgs);

        // 将消息保存到全局用户管理器的聊天记录中
        UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);

        return;
    }

    // 如果聊天列表中还没有这个好友的项，创建新的 ChatUserWid 项
    auto* chat_user_wid = new ChatUserWid();

    // 获取好友信息（昵称、头像等）
    auto fi_ptr = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
    chat_user_wid->SetInfo(fi_ptr); // 设置好友信息

    // 创建一个列表项并设置大小
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());

    // 设置显示的最后一条聊天记录
    chat_user_wid->updateLastMsg(msg->_chat_msgs);

    // 保存聊天记录到缓存
    UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);

    // 插入到聊天用户列表顶部（最近活跃）
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    // 将该用户的 UID 与列表项绑定，方便下次快速查找
    _chat_items_added.insert(msg->_from_uid, item);
}



bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    // 判断事件类型是否为鼠标点击事件
    if (event->type() == QEvent::MouseButtonPress) {
        // 将事件强制转换为鼠标事件类型
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // 调用自定义的鼠标点击处理函数，统一处理点击事件
        handleGlobalMousePress(mouseEvent);
    }

    // 将事件传回基类处理（保持默认事件行为）
    return QDialog::eventFilter(watched, event);
}


void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    // 实现点击位置的判断和处理逻辑
    // 先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if( _mode != ChatUIMode::SearchMode){
        return;
    }

    // 将鼠标点击位置转换为搜索列表坐标系中的位置
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    // 判断点击位置是否在聊天列表的范围内
    if (!ui->search_list->rect().contains(posInSearchList)) {
        // 如果不在聊天列表内，清空输入框
        ui->search_edit->clear();
        ShowSearch(false);
    }
}

void ChatDialog::CloseFindDlg()
{
    ui->search_list->CloseFindDlg();
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata)
{
    for(auto & msg : msgdata){
        if(msg->_from_uid != _cur_chat_uid){
            break;
        }

        ui->chat_page->AppendChatMsg(msg);
    }
}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata) {
    // 如果当前没有聊天对象（没有选中好友），直接返回
    if (_cur_chat_uid == 0) {
        return;
    }

    // 查找当前聊天用户在聊天用户列表中的 item
    auto find_iter = _chat_items_added.find(_cur_chat_uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    // 从 QListWidgetItem 中获取绑定的自定义 QWidget
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    // 强转为基础类 ListItemBase，用于统一处理
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    // 判断类型是否为聊天用户项（不是群提示等）
    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        // 强转为 ChatUserWid 类型，获取用户信息
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        // 获取当前聊天用户的信息，追加发送的消息
        auto user_info = con_item->GetUserInfo();
        user_info->_chat_msgs.push_back(msgdata);

        // 构造一个 vector，用于统一接口的处理（模拟一次性发送多个）
        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msgdata);

        // 追加消息到 UserMgr 管理器中，保存聊天记录
        UserMgr::GetInstance()->AppendFriendChatMsg(_cur_chat_uid, msg_vec);
        return;
    }
}


void ChatDialog::AddLBGroup(StateWidget* lb)
{
    _lb_list.push_back(lb);
}



void ChatDialog::addChatUserList()
{
    // 获取当前分页下的聊天好友列表（例如：每页加载 20 个）
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();

    // 如果列表不为空，遍历加载每个好友项
    if (!friend_list.empty()) {
        for (auto &friend_ele : friend_list) {
            // 如果该好友已被添加过（存在于_map中），则跳过
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if (find_iter != _chat_items_added.end()) {
                continue;
            }

            // 创建一个聊天用户展示控件（ChatUserWid）
            auto *chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info); // 设置用户数据

            // 新建一个 QListWidgetItem 并设置高度
            QListWidgetItem *item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint());

            // 添加 item 到界面上，并绑定上自定义 widget
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);

            // 插入映射表（便于后续查找和更新）
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        // 更新“已加载聊天项的数量”到 UserMgr（分页加载控制）
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }

    //模拟测试条目
    // 创建QListWidgetItem，并设置自定义的widget
//    for(int i = 0; i < 13; i++){
//        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
//        int str_i = randomValue%strs.size();
//        int head_i = randomValue%heads.size();
//        int name_i = randomValue%names.size();

//        auto *chat_user_wid = new ChatUserWid();
//        auto user_info = std::make_shared<UserInfo>(0,names[name_i],
//                                                    names[name_i],heads[head_i],0,strs[str_i]);
//        chat_user_wid->SetInfo(user_info);
//        QListWidgetItem *item = new QListWidgetItem;
//        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//        item->setSizeHint(chat_user_wid->sizeHint());
//        ui->chat_user_list->addItem(item);
//        ui->chat_user_list->setItemWidget(item, chat_user_wid);
//    }

}

void ChatDialog::loadMoreChatUser() {
    // 获取当前分页的一组聊天好友
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();

    if (!friend_list.empty()) {
        for (auto &friend_ele : friend_list) {
            // 如果当前用户已经在列表中，跳过
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if (find_iter != _chat_items_added.end()) {
                continue;
            }

            // 创建用户对应的聊天项UI
            auto *chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info);

            // 插入列表
            QListWidgetItem *item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);

            // 记录已插入的用户项
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        // 通知管理器已加载了一页
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }
}



void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto & ele: _lb_list){
        if(ele == lb){
            continue;
        }

        ele->ClearState();
    }
}

void ChatDialog::loadMoreConUser()
{
    // 获取当前分页的联系人
    auto friend_list = UserMgr::GetInstance()->GetConListPerPage();

    if (!friend_list.empty()) {
        for (auto &friend_ele : friend_list) {
            // 创建联系人项
            auto *chat_user_wid = new ConUserItem();
            chat_user_wid->SetInfo(friend_ele->_uid, friend_ele->_name, friend_ele->_icon);

            QListWidgetItem *item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->con_user_list->addItem(item);
            ui->con_user_list->setItemWidget(item, chat_user_wid);
        }

        // 通知已加载联系人数
        UserMgr::GetInstance()->UpdateContactLoadedCount();
    }
}

void ChatDialog::SetSelectChatItem(int uid)
{
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    // 如果 uid 为 0，默认选中第一个
    if (uid == 0) {
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem *firstItem = ui->chat_user_list->item(0);
        if (!firstItem) return;

        QWidget *widget = ui->chat_user_list->itemWidget(firstItem);
        if (!widget) return;

        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) return;

        _cur_chat_uid = con_item->GetUserInfo()->_uid;
        return;
    }

    // 查找 uid 对应的项
    auto find_iter = _chat_items_added.find(uid);
    if (find_iter == _chat_items_added.end()) {
        qDebug() << "uid " << uid << " not found, set current row 0";
        ui->chat_user_list->setCurrentRow(0);
        return;
    }

    // 设置当前选中项并记录当前聊天对象 uid
    ui->chat_user_list->setCurrentItem(find_iter.value());
    _cur_chat_uid = uid;
}


void ChatDialog::SetSelectChatPage(int uid)
{
    // 如果聊天列表为空，直接返回
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    // 如果uid为0，默认选中第一个用户
    if (uid == 0) {
        auto item = ui->chat_user_list->item(0);

        // 获取列表项对应的 widget
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        if (!widget) {
            return;
        }

        // 转换为 ChatUserWid 类型
        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) {
            return;
        }

        // 获取用户信息并设置到聊天页面
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);
        return;
    }

    // 查找uid对应的列表项
    auto find_iter = _chat_items_added.find(uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    // 获取该项绑定的 widget
    QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    // 转换为统一的基类指针
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    // 判断类型为聊天用户项
    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        // 获取用户信息并设置到聊天页面
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);
        return;
    }
}


void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDlg();
		ui->search_edit->clear();
		ui->search_edit->clearFocus();
    }
}

void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    loadMoreChatUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();
    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
    ClearLabelState(ui->side_chat_lb);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_side_contact(){
    ClearLabelState(ui->side_contact_lb);
    //设置
    if(_last_widget == nullptr){
        ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
        _last_widget = ui->friend_apply_page;
    }else{
        ui->stackedWidget->setCurrentWidget(_last_widget);
    }
    emit cancel_red();
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    if (!str.isEmpty()) {
        ShowSearch(true);
    }
}

void ChatDialog::slot_focus_out()
{
    ShowSearch(false);
}

void ChatDialog::slot_loading_contact_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    loadMoreConUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_switch_apply_friend_page()
{
    _last_widget = ui->friend_apply_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
}

void ChatDialog::slot_friend_info_page(std::shared_ptr<UserInfo> user_info)
{
    _last_widget = ui->friend_info_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_info_page);
    ui->friend_info_page->SetInfo(user_info);
}



void ChatDialog::slot_show_search(bool show)
{
    ShowSearch(show);
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "receive apply friend, applyuid is " << apply->_from_uid << " name is "<< apply->_name ;

    // 检查是否已经处理过该申请（是否已经有相同的申请）
    bool b_already = UserMgr::GetInstance()->AlreadyApply(apply->_from_uid);
    if(b_already){
        return;  // 如果已经处理过，直接返回
    }

    // 将新的申请信息添加到申请列表中
    UserMgr::GetInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));

    // 显示侧边栏和联系人列表中的红点，表示有新的申请
    ui->side_contact_lb->ShowRedPoint();
    ui->con_user_list->ShowRedPoint(true);

    // 在好友申请页面中添加新的申请信息
    ui->friend_apply_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info) {
    // 调试信息：显示接收到的验证信息
    // qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
    //          << " name is " << auth_info->_name << " nick is " << auth_info->_nick;

    // 判断用户是否已经是好友
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if (bfriend) {
        qDebug() << "已经是好友";  // 如果是好友，跳过此请求
        return;
    }

    // 将好友信息添加到好友列表中
    UserMgr::GetInstance()->AddFriend(auth_info);

    // 创建一个新的聊天用户界面，展示添加的好友信息
    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);

    // 创建新的 QListWidgetItem 并设置自定义的 widget
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());

    // 插入到聊天用户列表中
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    // 记录已添加的聊天项
    _chat_items_added.insert(auth_info->_uid, item);

    ui->con_user_list->ShowRedPoint(false);
    ui->side_contact_lb->ShowRedPoint();

}

/**
 * @brief ChatDialog::slot_auth_rsp
 * 槽函数：处理收到的加好友响应。
 * @param auth_rsp 包含加好友请求响应信息，包括用户的UID、名称和昵称等。
 */
void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    // 调试信息：显示接收到的加好友响应信息
    // qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
    //          << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

    // 判断该用户是否已经是好友
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if (bfriend) {
        qDebug() << "已经是好友...";  // 如果是好友，跳过此响应
        return;
    }

    // 将响应信息中的用户添加到好友列表中
    UserMgr::GetInstance()->AddFriend(auth_rsp);

    // 创建新的聊天用户界面，展示新添加的好友信息
    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info);

    // 创建新的 QListWidgetItem，并设置自定义的 widget
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());

    // 将该项插入到聊天用户列表的最前面
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    // 记录已添加的聊天项
    _chat_items_added.insert(auth_rsp->_uid, item);

    ui->con_user_list->ShowRedPoint(false);
}


void ChatDialog::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    auto find_iter = _chat_items_added.find(si->_uid);
    if(find_iter != _chat_items_added.end()){
        ui->chat_user_list->scrollToItem(find_iter.value());
        ui->side_chat_lb->SetSelected(true);
        SetSelectChatItem(si->_uid);
        //更新聊天界面信息
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(si->_uid, item);

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(si->_uid);
    //更新聊天界面信息
    SetSelectChatPage(si->_uid);
    slot_side_chat();

}

void ChatDialog::slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info)
{
    auto find_iter = _chat_items_added.find(user_info->_uid);
    if(find_iter != _chat_items_added.end()){
        ui->chat_user_list->scrollToItem(find_iter.value());
        ui->side_chat_lb->SetSelected(true);
        SetSelectChatItem(user_info->_uid);
        //更新聊天界面信息
        SetSelectChatPage(user_info->_uid);
        slot_side_chat();
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(user_info->_uid, item);
    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(user_info->_uid);
    //更新聊天界面信息
    SetSelectChatPage(user_info->_uid);
    slot_side_chat();
}

