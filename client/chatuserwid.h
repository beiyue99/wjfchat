#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include <QWidget>
#include "listitembase.h"
namespace Ui {
class ChatUserWid;
}

// 原本该类继承自QWidget，但是为了实现一些复用，我们自定义了Base类继承自QWidget
class ChatUserWid : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(250, 70); // 返回自定义的尺寸
    }

    void SetInfo(QString name, QString head, QString msg);

private:
    Ui::ChatUserWid *ui;
    QString _name;
    QString _head;
    QString _msg;
};

#endif // CHATUSERWID_H
