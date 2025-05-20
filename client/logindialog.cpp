#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include "httpmgr.h"
#include "tcpmgr.h"
#include <QRegExp>
#include <QRegularExpression>
#include <QPainter>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    //连接忘记密码槽函数
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
    //注册http请求处理函数
    initHttpHandlers();

    ui->pass_edit->setEchoMode(QLineEdit::Password);

    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);

    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::Inst(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::Inst(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登陆失败信号
    connect(TcpMgr::Inst(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);

    initHead();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::initHead()
{
    // 加载图片
    QPixmap originalPixmap(":/res/login.png");
      // 设置图片自动缩放

    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // 最后两个参数分别是x和y方向的圆角半径
    painter.setClipPath(path);

    // 将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置绘制好的圆角图片到QLabel上
    ui->head_label->setPixmap(roundedPixmap);

}

void LoginDialog::initHttpHandlers()
{
    // 注册 HTTP 登录请求的处理函数
    // 当登录请求响应返回后，HttpMgr 会调用这个 lambda 函数
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            // 登录返回错误，提示错误信息
            showTip(tr("参数错误"), false);
            enableBtn(true);
            return;
        }

        // 提取登录返回的用户信息
        QString email = jsonObj["email"].toString();
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        // 保存登录用户的 uid 和 token
        _uid = si.Uid;
        _token = si.Token;

        qDebug() << "email is " << email << " uid is " << si.Uid
                 << " host is " << si.Host << " Port is " << si.Port
                 << " Token is " << si.Token;

        // 登录成功后，发出信号，通知 TcpMgr 建立与聊天服务器的 TCP 长连接
        emit sig_connect_tcp(si);
    });
}


void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
         ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void LoginDialog::slot_forget_pwd()
{
    emit switchReset();
}

bool LoginDialog::checkUserValid(){

    auto email = ui->email_edit->text();
    if(email.isEmpty()){
        qDebug() << "email empty " ;
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
    return true;
}

void LoginDialog::on_login_btn_clicked()
{
    // 检查邮箱和密码是否合法
    if (!checkUserValid()) return;
    if (!checkPwdValid()) return;

    // 禁用按钮，避免重复点击
    enableBtn(false);

    // 获取用户输入的邮箱和密码
    auto email = ui->email_edit->text();
    auto pwd = ui->pass_edit->text();

    // 构建登录请求的 JSON 数据
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd); // 对密码进行加密传输

    // 发送 HTTP 登录请求，模块类型为 LOGINMOD，请求 ID 为 ID_LOGIN_USER
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}


void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    // 登录模块 HTTP 响应处理函数
    // 如果网络请求出错，提示错误
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("网络请求错误"), false);
        return;
    }

    // 尝试解析 JSON 响应字符串
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        showTip(tr("json解析错误"), false);
        return;
    }

    // 根据请求 ID 调用对应的 lambda 回调处理逻辑
    _handlers[id](jsonDoc.object());
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    // TCP 长连接建立成功后，继续向聊天服务器发送登录请求
    if (bsuccess) {
        showTip(tr("聊天服务连接成功，正在登录..."), true);

        // 构造包含 uid 和 token 的 JSON 数据
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

        // 发送聊天登录请求给聊天服务器，使用 TCP 长连接发送数据
        // ReqId::ID_CHAT_LOGIN 是表示“聊天模块登录”的请求类型
        // jsonData 包含用户 uid 和 token，在服务端验证通过后进入聊天模块
        emit TcpMgr::Inst()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonData);
    } else {
        // TCP 连接失败，提示网络异常
        showTip(tr("网络异常"), false);
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

void LoginDialog::AddTipErr(TipErr te,QString tips){
    _tip_errs[te] = tips;
    showTip(tips, false);
}


void LoginDialog::DelTipErr(TipErr te){
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }
    showTip(_tip_errs.first(), false);
}
