#include "searchlist.h"
#include<QScrollBar>
#include "adduseritem.h"
#include "invaliditem.h"
#include "findsuccessdlg.h"
#include "tcpmgr.h"
#include "customizeedit.h"
#include "findfaildlg.h"
#include "loadingdlg.h"
#include "userdata.h"
#include "usermgr.h"

SearchList::SearchList(QWidget *parent):QListWidget(parent), _find_dlg(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
    // 安装事件过滤器，用于处理滚动条显示和鼠标滚轮事件
    this->viewport()->installEventFilter(this);
    // 连接点击条目的信号和槽函数
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    // 添加默认提示项
    addTipItem();
    // 连接搜索请求的信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if (_find_dlg) {
        _find_dlg->hide(); // 隐藏查找对话框
        _find_dlg = nullptr; // 清空对话框指针
    }
}

void SearchList::SetSearchEdit(QWidget* edit) {
    _search_edit = edit; // 设置搜索框部件
}

bool SearchList::eventFilter(QObject *watched, QEvent *event)
{
    // 检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            // 鼠标悬浮，显示垂直滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开，隐藏垂直滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    // 检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数

        // 设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
        return true; // 停止事件传递
    }

    return QListWidget::eventFilter(watched, event);
}

void SearchList::waitPending(bool pending)
{
    if (pending) {
        // 显示加载对话框并标记为等待状态
        _loadingDialog = new LoadingDlg(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = pending;
    } else {
        // 隐藏并删除加载对话框，取消等待状态
        _loadingDialog->hide();
        _loadingDialog->deleteLater();
        _send_pending = pending;
    }
}

void SearchList::addTipItem()
{
    // 添加无效条目提示项
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    item_tmp->setSizeHint(QSize(250, 10)); // 设置无效项的大小
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item); // 设置无效项的控件
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable); // 设置不可选择

    // 添加“添加用户”提示项
    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(add_user_item->sizeHint()); // 设置“添加用户”项的大小
    this->addItem(item);
    this->setItemWidget(item, add_user_item); // 设置“添加用户”项的控件
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    // 获取点击的项的控件
    QWidget *widget = this->itemWidget(item);
    if (!widget) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 将自定义控件转化为基类类型 ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 获取当前项的类型
    auto itemType = customItem->GetItemType();
    if (itemType == ListItemType::INVALID_ITEM) {
        return; // 无效项，直接返回
    }

    if (itemType == ListItemType::ADD_USER_TIP_ITEM) {
        // 如果是添加用户提示项
        if (_send_pending) {
            return; // 如果处于等待状态，直接返回
        }

        if (!_search_edit) {
            return; // 如果没有搜索框，直接返回
        }

        waitPending(true); // 显示加载对话框

        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto uid_str = search_edit->text(); // 获取用户输入的UID

        // 发送请求给服务器查找用户
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 通过TCP发送请求
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
        return;
    }

    // 清除查找对话框
    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false); // 隐藏加载对话框

    if (si == nullptr) {
        // 查找失败，显示失败对话框
        _find_dlg = std::make_shared<FindFailDlg>(this);
    } else {
        // 查找到用户
        auto self_uid = UserMgr::GetInstance()->GetUid();
        if (si->_uid == self_uid) {
            qDebug() << "this is myself!"; // 如果是自己，直接返回
            return;
        }

        // 查找是否已经是好友
        bool bExist = UserMgr::GetInstance()->CheckFriendById(si->_uid);
        if (bExist) {
            // 已是好友，跳转到聊天界面
            emit sig_jump_chat_item(si);
            return;
        }

        // 处理未添加好友的情况
        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si); // 设置查找结果信息
    }

    _find_dlg->show(); // 显示查找结果对话框
}
