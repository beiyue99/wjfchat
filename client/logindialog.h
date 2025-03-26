#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class LoginDialog;
}

/**
 * @brief 登录对话框类
 * 继承自 QDialog，提供用户登录界面，包括用户输入校验、错误提示、网络请求处理等功能。
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief LoginDialog 构造函数
     * @param parent 父窗口指针，默认为 nullptr
     */
    explicit LoginDialog(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     * 释放 UI 资源
     */
    ~LoginDialog();

private:
    /**
     * @brief 初始化头像
     * 用于加载或设置用户头像
     */
    void initHead();

    /**
     * @brief 校验用户输入是否合法
     * @return true - 合法，false - 非法
     */
    bool checkUserValid();

    /**
     * @brief 校验密码输入是否合法
     * @return true - 合法，false - 非法
     */
    bool checkPwdValid();

    /**
     * @brief 添加错误提示
     * @param te 错误类型（TipErr 枚举值）
     * @param tips 错误提示信息
     */
    void AddTipErr(TipErr te, QString tips);

    /**
     * @brief 删除指定错误提示
     * @param te 错误类型
     */
    void DelTipErr(TipErr te);

    /**
     * @brief 显示提示信息
     * @param str 提示内容
     * @param b_ok 是否是成功提示（true 为成功，false 为错误）
     */
    void showTip(QString str, bool b_ok);

    /**
     * @brief 控制按钮的启用/禁用
     * @param enabled 是否启用按钮
     * @return 返回按钮当前是否可用
     */
    void enableBtn(bool enabled);

    /**
     * @brief 初始化 HTTP 处理函数
     * 用于处理网络请求的响应回调
     */
    void initHttpHandlers();

    Ui::LoginDialog *ui;  ///< UI 界面指针

    QMap<TipErr, QString> _tip_errs;  ///< 存储错误信息的映射表（错误类型 -> 错误提示）

    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;  ///< HTTP 请求处理映射

    int _uid;      ///< 登录用户的 UID
    QString _token; ///< 登录成功后服务器返回的 Token

public slots:
    /**
     * @brief 忘记密码按钮点击槽函数
     * 处理用户点击忘记密码时的逻辑
     */
    void slot_forget_pwd();

    /**
     * @brief 处理登录流程完成的槽函数
     * @param id 请求 ID
     * @param res 服务器返回的响应数据（JSON 格式字符串）
     * @param err 错误码
     */
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);


    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int);

signals:
    /**
     * @brief 切换到注册界面
     */
    void switchRegist();

    /**
     * @brief 切换到密码重置界面
     */
    void switchReset();

    /**
     * @brief 发送服务器信息信号，通知 TCP 连接建立
     * @param serverInfo 服务器信息结构体
     */
    void sig_connect_tcp(ServerInfo serverInfo);

private slots:
    /**
     * @brief 登录按钮点击事件槽函数
     */
    void on_login_btn_clicked();
};

#endif // LOGINDIALOG_H
