#ifndef STATEWIDGET_H
#define STATEWIDGET_H

#include <QWidget>
#include "global.h"
#include <QLabel>

class StateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StateWidget(QWidget *parent = nullptr);

    // 设置各种状态下的样式图片路径（正常、悬浮、按下、选中等）
    void SetState(QString normal="", QString hover="", QString press="",
                  QString select="", QString select_hover="", QString select_press="");

    // 获取当前控件状态
    ClickLbState GetCurState();

    // 清除当前状态并刷新界面
    void ClearState();

    // 设置当前是否为选中状态
    void SetSelected(bool bselected);

    // 添加一个红点提示控件
    void AddRedPoint();

    // 控制红点显示或隐藏
    void ShowRedPoint();

protected:
    // 绘制控件内容
    void paintEvent(QPaintEvent* event);

    // 处理鼠标按下事件，改变控件状态
    virtual void mousePressEvent(QMouseEvent *ev) override;

    // 处理鼠标释放事件，发出点击信号
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;

    // 处理鼠标进入控件事件，切换到悬浮状态
    virtual void enterEvent(QEvent* event) override;

    // 处理鼠标离开控件事件，还原状态
    virtual void leaveEvent(QEvent* event) override;

private:
    // 普通状态样式
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    // 选中状态样式
    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    // 当前状态
    ClickLbState _curstate;

    // 红点提示控件
    QLabel * _red_point;

signals:
    // 点击控件时发出信号
    void clicked(void);

public slots:
    // 取消红点显示
    void slot_on_cancel_red();
};

#endif // STATEWIDGET_H
