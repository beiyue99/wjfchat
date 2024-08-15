#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    // 设置窗口图标
    setWindowIcon(QIcon(":/icon.png"));
    _login_dlg=new LoginDialog(this);
    setCentralWidget(_login_dlg);
//    _login_dlg->show();
    //连接信号与槽，实现注册的跳转
    connect(_login_dlg,&LoginDialog::switchRegist,this,&MainWindow::SlotSwitchReg);
    _reg_dlg=new RegistDialog(this);

    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    //会直接显示，不需手动show
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    _reg_dlg->hide();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

