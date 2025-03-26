#ifndef REGISTDIALOG_H
#define REGISTDIALOG_H

#include <QDialog>
#include "global.h"

// 定义 Ui 命名空间，用于访问界面组件
namespace Ui {
class RegistDialog;
}

class RegistDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：初始化注册对话框
    explicit RegistDialog(QWidget *parent = nullptr);

    // 析构函数：清理资源
    ~RegistDialog();

private slots:
    // 获取验证码按钮按下的回调函数
    void on_getCode_btn_clicked();

    // 处理网络请求结果的槽函数
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    // 确认按钮点击事件处理函数
    void on_sure_Btn_clicked();

    // 返回按钮点击事件处理函数
    void on_return_btn_clicked();

    // 取消按钮点击事件处理函数
    void on_cancel_Btn_clicked();

private:
    // 初始化 HTTP 请求处理函数
    void initHttpHandlers();

    // 显示提示信息函数
    void showTip(QString str, bool b_ok);

    // 界面元素的指针
    Ui::RegistDialog *ui;

    // 用户输入验证函数
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkConfirmValid();
    bool checkVarifyValid();

    // 添加错误提示信息
    void AddTipErr(TipErr te, QString tips);

    // 删除错误提示信息
    void DelTipErr(TipErr te);

    // 更改提示信息页面
    void ChangeTipPage();

    // 错误提示信息的映射
    QMap<TipErr, QString> _tip_errs;

    // 网络请求响应处理器映射
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    // 倒计时定时器
    QTimer* _countdown_timer;

    // 倒计时剩余时间
    int _countdown;

signals:
    // 切换到登录界面信号
    void sigSwitchLogin();
};

#endif // REGISTDIALOG_H
