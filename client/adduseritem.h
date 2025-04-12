#ifndef ADDUSERITEM_H
#define ADDUSERITEM_H


// 好友列表中一个用户项
#include <QWidget>
#include "listitembase.h"
namespace Ui {
class AddUserItem;
}

class AddUserItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit AddUserItem(QWidget *parent = nullptr);
    ~AddUserItem();
    QSize sizeHint() const override {
        return QSize(250, 70); // 返回自定义的尺寸
    }
protected:

private:
    Ui::AddUserItem *ui;
};

#endif // ADDUSERITEM_H
