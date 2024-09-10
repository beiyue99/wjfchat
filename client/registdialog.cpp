#include "registdialog.h"
#include "ui_registdialog.h"
#include"global.h"
#include "httpmgr.h"
RegistDialog::RegistDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegistDialog)
{
    ui->setupUi(this);
    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->comit_Edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state","normal");
    repolish(ui->err_tip);
    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_reg_mod_finish,
            this,&RegistDialog::slot_reg_mod_finish);
    initHttpHandlers();
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
        showTip(tr("验证码已发送到邮箱，注意查收！"),true);
        return;
    }
    //jsonDoc.object();//转化为json对象
    _handlers[id](jsonDoc.object());  // 将解析的 JSON 对象交给相应的处理器
}

void RegistDialog::initHttpHandlers()
{
    //注册获取验证码回包的逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE,[this](const QJsonObject& jsonObj){
        int error=jsonObj["error"].toInt();
        if(error!=ErrorCodes::SUCCESS){
            showTip(tr("参数错误！"),false);
            return  ;
        }
        auto email=jsonObj["email"].toString();
        showTip(tr("邮验证码已经发送至邮箱，注意查收"),true);
        qDebug()<<"email is"<<email;
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
