#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H

#include <QWidget>
#include "global.h"

// 所有列表项控件的基类，用于统一接口和类型标识
class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget *parent = nullptr);

    // 设置当前 item 类型
    void SetItemType(ListItemType itemType);

    // 获取当前 item 类型
    ListItemType GetItemType();

protected:
    // 重绘控件样式（为支持 QSS）
    void paintEvent(QPaintEvent* event) override;

private:
    // 当前 item 类型
    ListItemType _itemType;
};

#endif // LISTITEMBASE_H
