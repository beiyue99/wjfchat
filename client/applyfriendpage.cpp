#include "applyfriendpage.h"
#include "ui_applyfriendpage.h"
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QRandomGenerator>
#include "applyfrienditem.h"
#include "authenfriend.h"
#include "applyfriend.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include "chatdialog.h"


ApplyFriendPage::ApplyFriendPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyFriendPage)
{
    ui->setupUi(this);
    connect(ui->apply_friend_list, &ApplyFriendList::sig_show_search, this, &ApplyFriendPage::sig_show_search);
    loadApplyList();
    //接受tcp传递的authrsp信号处理
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ApplyFriendPage::slot_auth_rsp);
}

ApplyFriendPage::~ApplyFriendPage()
{
    delete ui;
}

// 添加新的好友申请
void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply)
{

    // 创建一个新的好友申请项
    auto* apply_item = new ApplyFriendItem();

    // 创建好友申请的详细信息，并传入头像、用户名等信息
    auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid,
             apply->_name, apply->_desc, apply->_icon, apply->_nick, apply->_sex, 0);

    // 设置好友申请项的详细信息
    apply_item->SetInfo(apply_info);

    // 创建一个新的列表项，用于在好友申请列表中插入
    QListWidgetItem* item = new QListWidgetItem;

    // 设置列表项的尺寸提示
    item->setSizeHint(apply_item->sizeHint());

    // 设置列表项的标志，不允许该项被选中或启用
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);

    // 将列表项插入到好友申请列表的顶部
    ui->apply_friend_list->insertItem(0, item);

    // 设置列表项的界面为 `apply_item`（即好友申请项的UI）
    ui->apply_friend_list->setItemWidget(item, apply_item);

    // 显示好友申请项的添加按钮
    apply_item->ShowAddBtn(true);

    // 将当前申请项存入 `unauth_items` 映射表中，key为发起申请的用户ID
    _unauth_items[apply->_from_uid] = apply_item;

    // 收到好友申请后，连接审核好友的信号
    connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
        // 创建一个新的审核好友界面，并传入申请信息
        auto* authFriend = new AuthenFriend(this);
        authFriend->setModal(true); // 设置为模态窗口
        authFriend->SetApplyInfo(apply_info); // 设置好友申请信息
        authFriend->show(); // 显示审核界面
    });
}





void ApplyFriendPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


//加载并展示所有的好友申请列表
void ApplyFriendPage::loadApplyList()
{
    // 获取用户的所有好友申请列表
    auto apply_list = UserMgr::GetInstance()->GetApplyList();

    // 遍历所有的申请项
    for(auto &apply: apply_list){
        // 生成0到99之间的随机整数，用于模拟头像（未来可以从服务器获取真实头像）
//        int randomValue = QRandomGenerator::global()->bounded(100);
//        int head_i = randomValue % heads.size(); // 从头像数组中选择一个头像

        // 创建一个新的好友申请项（ApplyFriendItem）
        auto* apply_item = new ApplyFriendItem();

        // 设置头像
        apply->SetIcon(apply->_icon);

        // 设置申请项的详细信息
        apply_item->SetInfo(apply);

        // 创建一个新的 QListWidgetItem（列表项）
        QListWidgetItem* item = new QListWidgetItem;

        // 设置列表项的尺寸提示
        item->setSizeHint(apply_item->sizeHint());

        // 设置列表项为不可选中和不可启用
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);

        // 将列表项插入到申请列表的顶部
        ui->apply_friend_list->insertItem(0, item);

        // 设置列表项的显示内容为 apply_item
        ui->apply_friend_list->setItemWidget(item, apply_item);

        // 如果申请状态为已处理（_status非零），隐藏添加按钮
        if(apply->_status){
            apply_item->ShowAddBtn(false);
        } else {
            // 如果申请状态为未处理，显示添加按钮，并将其添加到 _unauth_items 映射表中
            apply_item->ShowAddBtn(true);
            auto uid = apply_item->GetUid();
            _unauth_items[uid] = apply_item;
        }

        // 收到好友申请审核信号时，弹出审核界面
        connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
            auto* authFriend = new AuthenFriend(this);
            authFriend->setModal(true);  // 设置为模态窗口
            authFriend->SetApplyInfo(apply_info);  // 设置申请信息
            authFriend->show();  // 显示审核界面
        });
    }

    // 模拟假数据，创建QListWidgetItem，并设置自定义的widget
//    for(int i = 0; i < 13; i++){
//        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
//        int str_i = randomValue%strs.size();
//        int head_i = randomValue%heads.size();
//        int name_i = randomValue%names.size();

//        auto *apply_item = new ApplyFriendItem();
//        auto apply = std::make_shared<ApplyInfo>(0, names[name_i], strs[str_i],
//                                    heads[head_i], names[name_i], 0, 1);
//        apply_item->SetInfo(apply);
//        QListWidgetItem *item = new QListWidgetItem;
//        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//        item->setSizeHint(apply_item->sizeHint());
//        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
//        ui->apply_friend_list->addItem(item);
//        ui->apply_friend_list->setItemWidget(item, apply_item);
//        //收到审核好友信号
//        connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info){
//            auto *authFriend =  new AuthenFriend(this);
//            authFriend->setModal(true);
//            authFriend->SetApplyInfo(apply_info);
//            authFriend->show();
//        });
//    }
}


//收到一个认证响应时.找到对应的未认证申请项，并隐藏该申请项的“添加好友”按钮
void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp) {
    auto uid = auth_rsp->_uid;
    auto find_iter = _unauth_items.find(uid);
    find_iter->second->ShowAddBtn(false);
}





