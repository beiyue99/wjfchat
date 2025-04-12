#ifndef BUBBLE_H
#define BUBBLE_H

#include <QFrame>
#include "global.h"
#include <QHBoxLayout>

//创建一个自定义的消息气泡框架，用来显示聊天消息的外观。
class BubbleFrame : public QFrame
{
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void setMargin(int margin);
    void setWidget(QWidget *w);
protected:
    void paintEvent(QPaintEvent *e);
private:
    QHBoxLayout *m_pHLayout;
    ChatRole m_role;
     int      m_margin;
};

#endif // BUBBLE_H
