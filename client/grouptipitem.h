#ifndef GROUPTIPITEM_H
#define GROUPTIPITEM_H

#include <QWidget>
#include "listitembase.h"

namespace Ui {
class GroupTipItem;
}


class GroupTipItem : public ListItemBase
{
    Q_OBJECT

public:
    // 函数功能：构造函数，初始化群提示项控件
    explicit GroupTipItem(QWidget *parent = nullptr);

    // 函数功能：析构函数，释放UI资源
    ~GroupTipItem();

    // 函数功能：返回该控件在列表中应占据的大小（用于设置QListWidgetItem大小）
    QSize sizeHint() const override;

    // 函数功能：设置提示文本
    void SetGroupTip(QString str);

private:
    // 当前提示内容
    QString _tip;

    // UI界面指针
    Ui::GroupTipItem *ui;
};

#endif // GROUPTIPITEM_H
