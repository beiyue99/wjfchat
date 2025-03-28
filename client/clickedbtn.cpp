#include "clickedbtn.h"
#include "global.h"

// 构造函数，设置鼠标悬停时的光标样式为手型
ClickedBtn::ClickedBtn(QWidget *parent):QPushButton (parent)
{
      setCursor(Qt::PointingHandCursor);
      setFocusPolicy(Qt::NoFocus);
}

ClickedBtn::~ClickedBtn(){

}

// 设置按钮的不同状态图片，并更新样式
void ClickedBtn::SetState(QString normal, QString hover, QString press)
{
    _hover = hover;
    _normal = normal;
    _press = press;
    setProperty("state",normal);
    repolish(this);
    update();
}

// 处理鼠标进入事件，设置按钮的状态为悬停状态并更新样式
void ClickedBtn::enterEvent(QEvent *event)
{
    setProperty("state",_hover);
    repolish(this);
    update();
    QPushButton::enterEvent(event);
}

void ClickedBtn::leaveEvent(QEvent *event)
{
    setProperty("state",_normal);
    repolish(this);
    update();
    QPushButton::leaveEvent(event);
}

// 处理鼠标按下事件，设置按钮的状态为按下状态并更新样式
void ClickedBtn::mousePressEvent(QMouseEvent *event)
{
    setProperty("state",_press);
    repolish(this);
    update();
    QPushButton::mousePressEvent(event);
}

// 处理鼠标释放事件，设置按钮的状态为悬停状态并更新样式
void ClickedBtn::mouseReleaseEvent(QMouseEvent *event)
{
    setProperty("state",_hover);
    repolish(this);
    update();
    QPushButton::mouseReleaseEvent(event);
}
