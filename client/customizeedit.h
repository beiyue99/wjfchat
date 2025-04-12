#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H

#include <QLineEdit>
#include <QDebug>

// CustomizeEdit 类继承自 QLineEdit，提供自定义的行为，如最大长度限制和失去焦点时的信号
class CustomizeEdit : public QLineEdit
{
    Q_OBJECT

public:
    // 构造函数，初始化 CustomizeEdit 控件
    explicit CustomizeEdit(QWidget *parent = nullptr);

    // 设置最大输入字符长度
    void SetMaxLength(int maxLen);

protected:
    // 重写失去焦点事件的处理函数
    // 当控件失去焦点时，会触发这个事件
    void focusOutEvent(QFocusEvent *event) override;
private:
    // 限制文本输入的最大长度
    void limitTextLength(QString text);

    // 最大输入字符长度
    int _max_len;

signals:
    // 失去焦点时触发的信号
    void sig_foucus_out();
};

#endif // CUSTOMIZEEDIT_H
