#include "registdialog.h"
#include "ui_registdialog.h"
#include"global.h"
#include "httpmgr.h"
RegistDialog::RegistDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegistDialog)
{
    ui->setupUi(this);
    // 设置密码和确认密码两项输入保密
    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->comit_Edit->setEchoMode(QLineEdit::Password);

      //设置自定义属性state的值为normal,默认为绿色
    ui->err_tip->setProperty("state","normal");

    // 调用自定义函数 repolish，用于重新应用样式表，更新 err_tip 的外观
    repolish(ui->err_tip);

    //http管理类发出注册完成的信号（携带服务器回应的数据），调用slot_reg_mod_finish处理
    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_reg_mod_finish,
            this,&RegistDialog::slot_reg_mod_finish);

    // 调用 initHttpHandlers 函数，初始化 HTTP 请求的处理函数
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
        //jsonDoc.object();//转化为json对象
        _handlers[id](jsonDoc.object());  // 将解析的 JSON 对象交给相应的处理器
    }

}


//注册消息处理器(根据请求id)
void RegistDialog::initHttpHandlers()
{
    // 注册请求验证码的消息处理器
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){

        // 从回包 JSON 对象中提取 "error" 字段的值，并将其转换为整型
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            // 显示错误提示，使用 showTip 函数并传入参数提示信息和 false（表示失败状态）
            showTip(tr("参数错误！"), false);
            return;
        }

        // 从回包中提取 "email" 字段的值，并将其转换为字符串
        auto email = jsonObj["email"].toString();

        // 显示成功提示信息，使用 showTip 函数并传入提示信息和 true（表示成功状态）
        showTip(tr("验证码已经发送至邮箱，注意查收"), true);

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
