#include "registdialog.h"
#include "ui_registdialog.h"
#include"global.h"
#include "httpmgr.h"
RegistDialog::RegistDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegistDialog),_countdown(5)
{
    ui->setupUi(this);
    // 设置密码和确认密码两项输入保密
    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->confirm_Edit->setEchoMode(QLineEdit::Password);

      //设置自定义属性state的值为normal,默认为绿色
    ui->err_tip->setProperty("state","normal");

    // 调用自定义函数 repolish，用于重新应用样式表，更新 err_tip 的外观
    repolish(ui->err_tip);

    //http管理类发出注册完成的信号（携带服务器回应的数据），调用slot_reg_mod_finish处理
    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_reg_mod_finish,
            this,&RegistDialog::slot_reg_mod_finish);

    // 调用 initHttpHandlers 函数，初始化 HTTP 请求的处理函数
    initHttpHandlers();

    ui->err_tip->clear();

    connect(ui->userEdit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });
    connect(ui->email_Edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->pass_Edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });
    connect(ui->confirm_Edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });
    connect(ui->varify_Edit, &QLineEdit::editingFinished, this, [this](){
            checkVarifyValid();
    });

    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                                "visible_hover","");
    ui->confirm_visible->SetState("unvisible","unvisible_hover","","visible",
                                    "visible_hover","");
    //连接点击事件
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_Edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->pass_Edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });
    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_Edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->confirm_Edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    // 创建定时器
    _countdown_timer = new QTimer(this);
    // 连接信号和槽
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown==0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_lb->setText(str);
    });
}

RegistDialog::~RegistDialog()
{
    delete ui;
}

void RegistDialog::on_getCode_btn_clicked()
{
    auto email=ui->email_Edit->text();
    //username@xxx.com
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match=regex.match(email).hasMatch();
    if(match)
    {
        //发送http验证码
        QJsonObject json_obj;
                json_obj["email"] = email;

                HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),
                                    json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::REGISTERMOD);

    }
    else
    {
        showTip(tr("邮箱地址格式不正确!"),false);
    }
}

void RegistDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err!=ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }

    //解析JSON字符串，res转化为QByteArray
    QJsonDocument jsonDoc=QJsonDocument::fromJson(res.toUtf8());
    //toUtf8 用于将 QString 对象转换为 UTF-8 编码的 QByteArray。
    //fromJson用于将json格式的QByteArray转化为QJsonDocument
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败！"),false);
        return;
    }
    if(jsonDoc.isObject()){
        //jsonDoc.object();//转化为json对象
        _handlers[id](jsonDoc.object());  // 将解析的 JSON 对象交给相应的处理器
    }

}



//根据请求Id，注册相关处理函数
void RegistDialog::initHttpHandlers()
{
    //注册请求验证码的消息处理器
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){

        // 从回包 JSON 对象中提取 "error" 字段的值，并将其转换为整型
        int error = jsonObj["error"].toInt();

        //检查 JSON 回包是否正确包含 error 和 email 字段
        qDebug() << "Response JSON:" << jsonObj;

        if (error != ErrorCodes::SUCCESS) {
            // 显示错误提示，使用 showTip 函数并传入参数提示信息和 false（表示失败状态）
            showTip(tr("参数错误！"), false);
            return;
        }

        // 从回包中提取 "email" 字段的值，并打印
        auto email = jsonObj["email"].toString();
        qDebug() << "email is" << email;

        // 显示成功提示信息，使用 showTip 函数并传入提示信息和 true（表示成功状态）
        showTip(tr("验证码已经发送至邮箱，注意查收"), true);
    });

    //注册用户回包逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug()<< "email is " << email ;
        qDebug()<< "user name is" << jsonObj["name"].toString();
        ChangeTipPage();
    });


}


void RegistDialog::showTip(QString str,bool b_ok)
{
    if(b_ok)
    {
        ui->err_tip->setProperty("state","normal");
    }
    else
    {
        ui->err_tip->setProperty("state","err");
    }
    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void RegistDialog::on_sure_Btn_clicked()
{
    if(ui->userEdit->text() == ""){
            showTip(tr("用户名不能为空"), false);
            return;
        }
        if(ui->email_Edit->text() == ""){
            showTip(tr("邮箱不能为空"), false);
            return;
        }
        if(ui->pass_Edit->text() == ""){
            showTip(tr("密码不能为空"), false);
            return;
        }
        if(ui->confirm_Edit->text() == ""){
            showTip(tr("确认密码不能为空"), false);
            return;
        }
        if(ui->confirm_Edit->text() != ui->pass_Edit->text()){
            showTip(tr("密码和确认密码不匹配"), false);
            return;
        }
        if(ui->varify_Edit->text() == ""){
            showTip(tr("验证码不能为空"), false);
            return;
        }
        //发送http请求注册用户
        QJsonObject json_obj;
        json_obj["user"] = ui->userEdit->text();
        json_obj["email"] = ui->email_Edit->text();
        json_obj["passwd"] = xorString(ui->pass_Edit->text());
        json_obj["confirm"] = xorString(ui->confirm_Edit->text());
        json_obj["varifycode"] = ui->varify_Edit->text();

        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                     json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}


void RegistDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}
void RegistDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }
    showTip(_tip_errs.first(), false);
}

void RegistDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    //启动定时器,设置间隔为1000毫秒(1秒)
    _countdown_timer->start(1000);
}

bool RegistDialog::checkUserValid()
{
    if(ui->userEdit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}
bool RegistDialog::checkPassValid()
{
    auto pass = ui->pass_Edit->text();
    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }
    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool RegistDialog::checkConfirmValid()
{
    auto pass = ui->pass_Edit->text();
    auto confirm = ui->confirm_Edit->text();
    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6~15"));
        return false;
    }
    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("不能包含非法字符"));
        return false;;
    }
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
    if(pass != confirm)
    {
        AddTipErr(TipErr::TIP_PWD_MISMATCH,tr("确认密码和密码不匹配"));
        return false;
    }
    else
    {
        DelTipErr(TIP_PWD_MISMATCH);
    }
    return true;
}
bool RegistDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_Edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegistDialog::checkVarifyValid()
{
    auto pass = ui->varify_Edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

void RegistDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegistDialog::on_cancel_Btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}


