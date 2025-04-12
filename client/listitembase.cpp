#include "listitembase.h"
#include <QStyleOption>
#include <QPainter>

// 构造函数，初始化控件
ListItemBase::ListItemBase(QWidget *parent) : QWidget(parent)
{
}

// 设置 item 类型
void ListItemBase::SetItemType(ListItemType itemType)
{
    _itemType = itemType;
}

// 获取 item 类型
ListItemType ListItemBase::GetItemType()
{
    return _itemType;
}

// 绘制样式支持 QSS，使样式表能正确应用到自定义控件
void ListItemBase::paintEvent(QPaintEvent *event)
{
    // 用于描述控件的样式信息
    QStyleOption opt;
    opt.init(this);  // 初始化样式选项，自动填充控件状态（焦点、启用状态等）

    // 在当前控件上创建绘图对象
    QPainter p(this);

    // 使用样式系统绘制控件的基本外观，确保QSS样式生效
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

