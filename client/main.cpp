#include "mainwindow.h"
#include "global.h"

#include<QFile>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){
        qDebug("open success!");
        QString style=QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
        qDebug("open failed!");
    }

    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();  //返回当前应用程序的目录路径。

    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                            QDir::separator() + fileName);
    //从 config.ini 配置文件中读取配置信息，并构建一个 URL 地址。
    QSettings settings(config_path, QSettings::IniFormat);  //QSettings用于读写配置文件
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://"+gate_host+":"+gate_port;


    MainWindow w;
    w.show();
    return a.exec();
}
