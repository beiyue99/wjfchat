#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include "tcpmgr.h"
#include <QtConcurrent/QtConcurrent>

class FileBubble : public QWidget
{
    Q_OBJECT
public:
    enum FileStatus {
        STATUS_SENDING,      // 正在上传
        STATUS_FINISHED,     // 已发送完
        STATUS_REJECTED      // 被对方拒绝
    };

    FileBubble(const QString& fileName, qint64 totalBytes, QWidget* parent = nullptr)
        : QWidget(parent),
          _total(totalBytes)
    {
        _icon  = new QLabel("📄");
        _name  = new QLabel(fileName);
        _progress   = new QProgressBar;
        _progress->setRange(0, 100);
        _send  = new QPushButton(u8"发送");

        auto *lay = new QHBoxLayout(this);
        lay->addWidget(_icon);
        lay->addWidget(_name, 1);
        lay->addWidget(_progress);
        lay->addWidget(_send);

        connect(_send, &QPushButton::clicked, this, &FileBubble::sig_send_clicked);
    }


    /* ★ 新增 */
       void SetFileStatus(FileStatus st)
       {
           switch (st) {
           case STATUS_SENDING:
               _progress->show();
               _send->setEnabled(false);
               break;
           case STATUS_FINISHED:
               _progress->setValue(100);
               _send->setText(u8"发送完成");
               _send->setEnabled(false);
               break;
           case STATUS_REJECTED:
               _progress->hide();
               _send->setText(u8"已拒绝");
               _send->setEnabled(false);
               break;
           }
       }
signals:
    void sig_send_clicked();                 // 通知 ChatPage 开始真正发送

public slots:
    void slot_set_progress(qint64 bytes)
    {
        if (bytes < 0) {                // ① 统一的“完成”信号
            _progress->setValue(100);   //   撑到 100
//            _progress->hide();          //   或者 _percentLbl->hide();
            _send->setText(u8"发送完成");
            _send->show();           //   ✔/已完成 之类
            return;
        }

        int pct = int(bytes * 100 / _total);
        _progress->setValue(pct);
    }


private:
    QLabel*       _icon;
    QLabel*       _name;
    QProgressBar* _progress;
    QPushButton*  _send;
    qint64        _total;
};
