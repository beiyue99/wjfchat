#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class LoginDialog;
}

// 登录界面类，处理用户登录相关逻辑，如输入验证、发送请求、处理响应等
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数，初始化登录界面和各项设置
    explicit LoginDialog(QWidget *parent = nullptr);

    // 析构函数，释放资源
    ~LoginDialog();

private:
    // 初始化头像显示
    void initHead();

    // 初始化网络请求的响应处理函数
    void initHttpHandlers();

    // 显示提示信息，可根据状态显示成功或错误提示
    void showTip(QString str, bool b_ok);

    // 检查用户名是否合法
    bool checkUserValid();

    // 检查密码是否合法
    bool checkPwdValid();

    // 设置登录按钮是否启用
    bool enableBtn(bool);

    // 添加错误提示信息
    void AddTipErr(TipErr te, QString tips);

    // 删除指定错误提示信息
    void DelTipErr(TipErr te);

    Ui::LoginDialog *ui; // UI界面对象指针

    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers; // 请求处理函数映射表

    QMap<TipErr, QString> _tip_errs; // 错误码与提示语映射表

    int _uid;         // 登录成功后的用户ID
    QString _token;   // 登录成功后的用户令牌

private slots:
    // 忘记密码时的槽函数
    void slot_forget_pwd();

    // 点击登录按钮时的槽函数
    void on_login_btn_clicked();

    // 登录模块请求完成时的槽函数
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);

    // TCP连接完成时的槽函数
    void slot_tcp_con_finish(bool bsuccess);

    // 登录失败时的槽函数
    void slot_login_failed(int);

signals:
    // 发送切换到注册界面的信号
    void switchRegister();

    // 发送切换到重置密码界面的信号
    void switchReset();

    // 通知外部建立 TCP 连接
    void sig_connect_tcp(ServerInfo);
};

#endif // LOGINDIALOG_H
