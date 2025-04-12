#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include "global.h"

// 可点击的标签控件，支持多种视觉状态切换（普通/选中/悬停/按下等）
class ClickedLabel : public QLabel
{
    Q_OBJECT
public:
    // 构造函数
    ClickedLabel(QWidget* parent);

    // 鼠标点击事件处理
    virtual void mousePressEvent(QMouseEvent *ev) override;

    // 鼠标释放事件处理
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;

    // 鼠标进入事件处理
    virtual void enterEvent(QEvent* event) override;

    // 鼠标离开事件处理
    virtual void leaveEvent(QEvent* event) override;

    // 设置标签的各种状态样式
    void SetState(QString normal = "", QString hover = "", QString press = "",
                  QString select = "", QString select_hover = "", QString select_press = "");

    // 获取当前状态
    ClickLbState GetCurState();

    // 设置当前状态
    bool SetCurState(ClickLbState state);

    // 重置为普通状态
    void ResetNormalState();

protected:
    // 样式表字符串：普通状态
    QString _normal;

    // 样式表字符串：普通状态 - 悬停
    QString _normal_hover;

    // 样式表字符串：普通状态 - 按下
    QString _normal_press;

    // 样式表字符串：选中状态
    QString _selected;

    // 样式表字符串：选中状态 - 悬停
    QString _selected_hover;

    // 样式表字符串：选中状态 - 按下
    QString _selected_press;

    // 当前状态
    ClickLbState _curstate;

signals:
    // 标签被点击时发出信号，传出标签文本和当前状态
    void clicked(QString, ClickLbState);
};

#endif // CLICKEDLABEL_H
