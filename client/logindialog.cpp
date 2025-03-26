#include <QPainter>
#include "global.h"
#include "logindialog.h"
#include "ui_logindialog.h"
#include "httpmgr.h"
#include "tcpmgr.h"

/**
 * @brief LoginDialog 构造函数
 * 初始化 UI 组件，绑定信号槽，初始化 HTTP 处理逻辑
 */
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    // 连接“注册”按钮的点击信号，触发 switchRegist 信号（切换到注册界面）
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegist);

    // 设置“忘记密码”标签的状态
    ui->forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");

    // 连接“忘记密码”点击事件到 slot_forget_pwd 槽函数
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);

    // 初始化头像
    initHead();

    // 初始化 HTTP 处理逻辑
    initHttpHandlers();

    // 连接 HTTP 登录结果信号到 slot_login_mod_finish 槽函数
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);

    //连接tcp连接请求的信号和槽函数
     connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登录失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);
}

/**
 * @brief LoginDialog 析构函数
 * 释放 UI 资源
 */
LoginDialog::~LoginDialog()
{
    delete ui;
}

/**
 * @brief 初始化头像
 * 读取本地头像文件，并裁剪成圆角显示
 */
void LoginDialog::initHead()
{
    QPixmap originalPixmap(":/res/head_1.jpg");

    // 调整图片尺寸以适应 QLabel
    qDebug() << originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建透明的 QPixmap 作为圆角头像
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent);

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 创建圆角路径
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10);
    painter.setClipPath(path);

    // 绘制圆角头像
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置到 QLabel 上
    ui->head_label->setPixmap(roundedPixmap);
}

/**
 * @brief 显示提示信息
 * @param str 提示文本
 * @param b_ok 是否为成功提示（true: 正常状态，false: 错误状态）
 */
void LoginDialog::showTip(QString str, bool b_ok)
{
    ui->err_tip->setProperty("state", b_ok ? "normal" : "err");
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

/**
 * @brief 校验用户输入的邮箱是否有效
 * @return true - 合法，false - 非法（邮箱不能为空）
 */
bool LoginDialog::checkUserValid()
{
    auto email = ui->email_edit->text();
    if (email.isEmpty()) {
        qDebug() << "email empty";
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

/**
 * @brief 校验用户输入的密码是否符合格式要求
 * @return true - 合法，false - 非法（长度或格式错误）
 */
bool LoginDialog::checkPwdValid()
{
    auto pwd = ui->pass_edit->text();

    // 密码长度应在 6 到 15 之间
    if (pwd.length() < 6 || pwd.length() > 15) {
        qDebug() << "Pass length invalid";
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 使用正则表达式检查密码格式
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if (!match) {
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

/**
 * @brief 处理“忘记密码”按钮点击事件，发送切换到重置密码界面的信号
 */
void LoginDialog::slot_forget_pwd()
{
    qDebug() << "slot forget pwd";
    emit switchReset();
}

/**
 * @brief 处理 HTTP 登录请求的返回结果
 * @param id 请求 ID
 * @param res 服务器返回的 JSON 字符串
 * @param err 错误码
 */
void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("网络请求错误"), false);
        return;
    }

    // 解析 JSON 返回数据
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        showTip(tr("json解析错误"), false);
        return;
    }

    // 调用对应的处理函数
    _handlers[id](jsonDoc.object());
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    if(bsuccess){
          showTip(tr("聊天服务连接成功，正在登录..."),true);
          QJsonObject jsonObj;
          jsonObj["uid"] = _uid;
          jsonObj["token"] = _token;
          QJsonDocument doc(jsonObj);
          QByteArray jsonString = doc.toJson(QJsonDocument::Indented);
          //发送tcp请求给chat server
          emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString);
       }else{
          showTip(tr("网络异常"),false);
          enableBtn(true);
    }
}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1")
                             .arg(err);
    showTip(result,false);
    enableBtn(true);
}

/**
 * @brief 控制“登录”和“注册”按钮的启用/禁用状态
 * @param enabled 是否启用按钮
 */
void LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
}

/**
 * @brief 初始化 HTTP 处理逻辑，注册回调函数
 */
void LoginDialog::initHttpHandlers()
{
    // 处理登录请求的返回数据
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("参数错误"), false);
            enableBtn(true);
            return;
        }

        auto user = jsonObj["user"].toString();
        // 解析服务器返回的用户信息
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;
        qDebug() << "user is " << user
                 << "email is " << jsonObj["email"].toString()
                 << " uid is " << si.Uid
                 << " host is " << si.Host
                 << " Port is " << si.Port
                 << " Token is " << si.Token;

        // 发送信号通知 TCP 连接服务器
        emit sig_connect_tcp(si);
    });
}




/**
 * @brief 添加错误信息，并更新 UI 显示
 * @param te 错误类型
 * @param tips 错误提示信息
 */
void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

/**
 * @brief 删除指定的错误信息，并更新 UI 显示
 * @param te 错误类型
 */
void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {
        ui->err_tip->clear();
    } else {
        showTip(_tip_errs.first(), false);
    }
}

/**
 * @brief 处理“登录”按钮点击事件，校验输入并发送 HTTP 请求
 */
void LoginDialog::on_login_btn_clicked()
{
    if (!checkUserValid() || !checkPwdValid()) {
        return;
    }
    enableBtn(false);
    QJsonObject json_obj;
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pass_edit->text());

    // 发送 HTTP 登录请求
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}
