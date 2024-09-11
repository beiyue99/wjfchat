#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);


    // 设置窗口图标
    setWindowIcon(QIcon(":/res/icon.png"));

    //创建并设置登录界面为主窗口
    _login_dlg=new LoginDialog(this);
    setCentralWidget(_login_dlg);



    //连接信号与槽----  登录窗口发出界面切换的信号,this捕获,SlotSwitchReg处理
    connect(_login_dlg,&LoginDialog::switchRegist,this,&MainWindow::SlotSwitchReg);


    _reg_dlg=new RegistDialog(this);

    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    //FramelessWindowHint  :实现无框窗口
    //CustomizeWindowHint  :移除默认的系统窗口样式
    _reg_dlg->hide();

}

MainWindow::~MainWindow()
{
    delete ui;
}



//显示注册界面,隐藏登录界面
void MainWindow::SlotSwitchReg()
{
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

