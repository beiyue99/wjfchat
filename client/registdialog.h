#ifndef REGISTDIALOG_H
#define REGISTDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class RegistDialog;
}

class RegistDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegistDialog(QWidget *parent = nullptr);
    ~RegistDialog();

private slots:
    void on_getCode_btn_clicked();   //获取验证码的按钮按下的回调
    void slot_reg_mod_finish(ReqId id,QString res,ErrorCodes err);  //处理网络请求的结果
private:
    void initHttpHandlers();
    void showTip(QString str,bool b_ok);
    Ui::RegistDialog *ui;
    QMap<ReqId,std::function<void(const QJsonObject&)>> _handlers;
};

#endif // REGISTDIALOG_H
