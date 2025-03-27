#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addChatUserList();   //测试函数，添加用户聊天列表

private:
    void ShowSearch(bool bsearch = false);
    Ui::ChatDialog *ui;
    ChatUIMode _mode;    //不同的模式
    ChatUIMode _state;   //某个模式下的不同状态
    bool _b_loading;

public slots:
    void slot_loading_chat_user();
};

#endif // CHATDIALOG_H
