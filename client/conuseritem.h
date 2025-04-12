#ifndef CONUSERITEM_H
#define CONUSERITEM_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ConUserItem;
}

class ConUserItem : public ListItemBase
{
    Q_OBJECT

public:
    // 构造函数：初始化ConUserItem，设置父窗口
    explicit ConUserItem(QWidget *parent = nullptr);

    // 析构函数：释放资源
    ~ConUserItem();

    // 获取该控件的推荐大小
    QSize sizeHint() const override;

    // 设置用户信息（通过AuthInfo对象）
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);

    // 设置用户信息（通过AuthRsp对象）
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);

    // 设置用户信息（通过uid、name和icon）
    void SetInfo(int uid, QString name, QString icon);

    // 显示或隐藏红点（通常用来表示未读消息或待处理请求）
    void ShowRedPoint(bool show = false);

    // 获取当前用户信息
    std::shared_ptr<UserInfo> GetInfo();

private:
    Ui::ConUserItem *ui;  // UI界面
    std::shared_ptr<UserInfo> _info;  // 存储用户信息
};

#endif // CONUSERITEM_H
