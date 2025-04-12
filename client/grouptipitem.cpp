#include "grouptipitem.h"
#include "ui_grouptipitem.h"

// 构造函数，初始化 UI 并设置该项的类型为群提示项
GroupTipItem::GroupTipItem(QWidget *parent)
    : ListItemBase(parent), _tip(""), ui(new Ui::GroupTipItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::GROUP_TIP_ITEM);
}

// 析构函数，释放 UI 资源
GroupTipItem::~GroupTipItem()
{
    delete ui;
}

// 设置该控件在列表中应显示的大小
QSize GroupTipItem::sizeHint() const
{
    return QSize(250, 25); // 返回固定大小
}

// 提示文本内容
void GroupTipItem::SetGroupTip(QString str)
{
    ui->label->setText(str);
}
