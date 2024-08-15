#include "registdialog.h"
#include "ui_registdialog.h"

RegistDialog::RegistDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegistDialog)
{
    ui->setupUi(this);
    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->comit_Edit->setEchoMode(QLineEdit::Password);
}

RegistDialog::~RegistDialog()
{
    delete ui;
}
