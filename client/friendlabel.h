#ifndef FRIENDLABEL_H
#define FRIENDLABEL_H

#include <QFrame>
#include <QString>

namespace Ui {
class FriendLabel;
}

/*
 * 类功能：用于在添加好友界面中展示标签，如“同事”“家人”等，支持显示文本与关闭功能
 */
class FriendLabel : public QFrame
{
    Q_OBJECT

public:
    // 函数功能：构造函数，初始化标签控件
    explicit FriendLabel(QWidget *parent = nullptr);

    // 函数功能：析构函数，释放UI资源
    ~FriendLabel();

    // 函数功能：设置标签文本
    void SetText(QString text);

    // 函数功能：获取标签宽度
    int Width();

    // 函数功能：获取标签高度
    int Height();

    // 函数功能：获取标签的文本内容
    QString Text();

private:
    // UI界面指针
    Ui::FriendLabel *ui;

    // 存储当前标签的文本
    QString _text;

    // 标签宽度
    int _width;

    // 标签高度
    int _height;

public slots:
    // 函数功能：槽函数，响应点击关闭按钮的操作
    void slot_close();

signals:
    // 信号功能：发送关闭信号，通知外部删除该标签，携带当前标签文本
    void sig_close(QString);
};

#endif // FRIENDLABEL_H
