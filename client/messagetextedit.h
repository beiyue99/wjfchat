#ifndef MESSAGETEXTEDIT_H
#define MESSAGETEXTEDIT_H

#include <QObject>
#include <QTextEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMimeType>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QPainter>
#include <QVector>
#include "global.h"

// 自定义的文本编辑器类，用于消息输入，继承自 QTextEdit，支持文本、图片、文件的插入与拖拽
class MessageTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    // 构造函数，初始化编辑器属性
    explicit MessageTextEdit(QWidget *parent = nullptr);

    // 析构函数
    ~MessageTextEdit();

    // 获取当前编辑器中用户输入的所有消息内容（包括图片、文本、文件）
    QVector<MsgInfo> getMsgList();

    // 从文件路径列表中插入文件内容（可用于拖拽文件或粘贴文件）
    void insertFileFromUrl(const QStringList &urls);

signals:
    // 当用户按下 Enter 键时发出该信号，表示用户尝试发送消息
    void send();

protected:
    // 拖拽进入编辑框时的处理（判断是否接受拖拽）
    void dragEnterEvent(QDragEnterEvent *event) override;

    // 拖拽释放时的处理（执行实际的文件插入）
    void dropEvent(QDropEvent *event) override;

    // 处理键盘输入事件，拦截 Enter 键用于发送消息
    void keyPressEvent(QKeyEvent *e) override;

private:
    // 插入图片文件（以文件路径 url 形式）
    void insertImages(const QString &url);

    // 插入文本文件（只展示部分文本内容）
    void insertTextFile(const QString &url);

    // 判断当前 mime 数据是否支持插入
    bool canInsertFromMimeData(const QMimeData *source) const override;

    // 从 mime 数据中插入内容（支持粘贴文件或图片）
    void insertFromMimeData(const QMimeData *source) override;

private:
    // 判断指定路径的文件是否为图片类型
    bool isImage(QString url);

    // 将一组消息插入消息列表中，并更新展示
    void insertMsgList(QVector<MsgInfo> &list, QString flag, QString text, QPixmap pix);

    // 从富文本中提取路径地址（例如粘贴文件时解析 file:// 开头的路径）
    QStringList getUrl(QString text);

    // 获取文件图标并将其绘制成 Pixmap，用于显示在编辑框中
    QPixmap getFileIconPixmap(const QString &url);

    // 根据文件大小（字节）转换成人类可读格式（如 KB、MB）
    QString getFileSize(qint64 size);

private slots:
    // 当文本变化时触发，用于触发界面更新或限制输入等
    void textEditChanged();

private:
    // 存储当前编辑器中用户已插入的消息对象列表
    QVector<MsgInfo> mMsgList;

    // 备份消息列表，用于提取或撤销时恢复
    QVector<MsgInfo> mGetMsgList;
};

#endif // MESSAGETEXTEDIT_H
