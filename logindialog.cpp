#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //点击注册按钮，会发出switchRegist信号
    connect(ui->reg_Btn,&QPushButton::clicked,this,&LoginDialog::switchRegist);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}
