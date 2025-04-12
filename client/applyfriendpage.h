#ifndef APPLYFRIENDPAGE_H
#define APPLYFRIENDPAGE_H

#include <QWidget>
#include "userdata.h"
#include <memory>
#include <QJsonArray>
#include <unordered_map>
#include "applyfrienditem.h"

namespace Ui {
class ApplyFriendPage;
}

class ApplyFriendPage : public QWidget
{
    Q_OBJECT

public:
    // 好友申请页面，负责展示所有好友申请的页面，管理申请项并处理响应
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();

    // 添加新的好友申请
    void AddNewApply(std::shared_ptr<AddFriendApply> apply);

protected:
    // 绘制界面
    void paintEvent(QPaintEvent *event) override;

private:
    // 加载申请列表
    void loadApplyList();

    Ui::ApplyFriendPage *ui;

    // 存储未认证的申请项，key 为用户 ID，value 为申请项
    std::unordered_map<int, ApplyFriendItem*> _unauth_items;

public slots:
    // 处理认证响应
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);

signals:
    // 显示或隐藏搜索框的信号
    void sig_show_search(bool);

};

#endif // APPLYFRIENDPAGE_H
