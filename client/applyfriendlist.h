#ifndef APPLYFRIENDLIST_H
#define APPLYFRIENDLIST_H

#include <QListWidget>
#include <QEvent>


//显示和管理好友申请列表
class ApplyFriendList : public QListWidget
{
    Q_OBJECT

public:
    // 好友申请列表
    ApplyFriendList(QWidget *parent = nullptr);

protected:
    // 事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    // 显示或隐藏搜索框的信号
    void sig_show_search(bool);
};

#endif // APPLYFRIENDLIST_H
