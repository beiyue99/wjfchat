#pragma once
#include <QWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

class RecvFileBubble : public QWidget
{
    Q_OBJECT
public:

    enum FileStatus {
        STATUS_TRANSFERRING,
        STATUS_FINISHED,
        STATUS_REJECTED
    };
    RecvFileBubble(const QString& fname, qint64 total, QWidget* parent = nullptr)
        : QWidget(parent), _total(total)
    {
        _icon = new QLabel(u8"📄");
        _name = new QLabel(fname);
        _progress  = new QProgressBar;  _progress->setRange(0,100);
        _accept = new QPushButton(u8"接受");
        _reject = new QPushButton(u8"拒绝");

        auto *lay = new QHBoxLayout(this);
        lay->addWidget(_icon);
        lay->addWidget(_name, 1);
        lay->addWidget(_progress);
        lay->addWidget(_accept);
        lay->addWidget(_reject);

        connect(_accept, &QPushButton::clicked, this, &RecvFileBubble::sig_accept);
        connect(_reject, &QPushButton::clicked, this, &RecvFileBubble::sig_reject);
    }


    void SetFileStatus(FileStatus status)
    {
        switch (status) {
        case STATUS_TRANSFERRING:
            _progress->show();
            _accept->setEnabled(true);
            _reject->setEnabled(true);
            break;
        case STATUS_FINISHED:
            _progress->setValue(100);
            _progress->hide();
            _accept->setText(u8"已完成");
            _accept->setEnabled(false);
            _reject->hide();  // 隐藏“拒绝”按钮
            break;
        case STATUS_REJECTED:
            _progress->hide();
            _accept->setText(u8"已拒绝");
            _accept->setEnabled(false);
            _reject->hide();
            break;
        }
    }

signals:
    void sig_accept();
    void sig_reject();

public slots:
    void slot_set_progress(qint64 bytes)
    {
        if (bytes < 0) {                // ① 统一的“完成”信号
            _progress->setValue(100);   //   撑到 100
            _progress->hide();          //   或者 _percentLbl->hide();
            _accept->setText(u8"已完成");
            _accept->show();           //   ✔/已完成 之类
            return;
        }

        int pct = int(bytes * 100 / _total);
        _progress->setValue(pct);
    }
private:
    QLabel* _icon; QLabel* _name; QProgressBar* _progress;
    QPushButton *_accept,*_reject; qint64 _total;
};
