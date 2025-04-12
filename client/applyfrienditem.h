#ifndef APPLYFRIENDITEM_H
#define APPLYFRIENDITEM_H

#include <QWidget>
#include <listitembase.h>
#include "userdata.h"
#include <memory>

namespace Ui {
class ApplyFriendItem;
}

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT

public:
    // 好友申请列表的一个好友申请项
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();

    // 设置好友申请信息
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);

    // 显示或隐藏添加按钮
    void ShowAddBtn(bool bshow);

    // 获取控件的推荐大小
    QSize sizeHint() const override {
        return QSize(250, 80);
    }

    // 获取用户 ID
    int GetUid();

private:
    Ui::ApplyFriendItem *ui;
    std::shared_ptr<ApplyInfo> _apply_info;
    bool _added;

signals:
    // 发送好友认证信号
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info);
};

#endif // APPLYFRIENDITEM_H
