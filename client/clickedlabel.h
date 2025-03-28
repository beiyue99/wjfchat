#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include "global.h"

/**
 * @brief ClickedLabel 类
 * 继承自 QLabel，扩展了鼠标点击、进入和离开事件的处理功能。
 * 允许设置不同状态（正常、悬停、按下、选中等）的样式，并提供信号 `clicked()` 用于通知点击事件。
 */
class ClickedLabel : public QLabel
{
    Q_OBJECT  // Qt 宏，启用信号与槽机制

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口，默认为 nullptr
     */
    ClickedLabel(QWidget* parent = nullptr);

    /**
     * @brief 处理鼠标按下事件
     * @param ev 鼠标事件
     */
    virtual void mousePressEvent(QMouseEvent *ev) override;

    virtual void mouseReleaseEvent(QMouseEvent *ev) override;

    /**
     * @brief 处理鼠标进入事件
     * @param event 事件对象
     */
    virtual void enterEvent(QEvent *event) override;

    /**
     * @brief 处理鼠标离开事件
     * @param event 事件对象
     */
    virtual void leaveEvent(QEvent *event) override;

    void SetState(QString normal = "", QString hover = "", QString press = "",
                  QString select = "", QString select_hover = "", QString select_press = "");

    /**
     * @brief 获取当前标签状态
     * @return 返回当前 ClickLbState 状态
     */
    ClickLbState GetCurState();
    bool SetCurState(ClickLbState state);
    void ResetNormalState();

private:
    QString _normal;          ///< 正常状态样式
    QString _normal_hover;    ///< 正常状态下鼠标悬停样式
    QString _normal_press;    ///< 正常状态下鼠标按下样式
    QString _selected;        ///< 选中状态样式
    QString _selected_hover;  ///< 选中状态下鼠标悬停样式
    QString _selected_press;  ///< 选中状态下鼠标按下样式
    ClickLbState _curstate;   ///< 当前标签状态

signals:
    /**
     * @brief clicked 信号
     * 当标签被点击时触发
     */
    void clicked(QString, ClickLbState);
};

#endif // CLICKEDLABEL_H
