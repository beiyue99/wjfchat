#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{

//    qSetMessagePattern("%{time hh:mm:ss.zzz} %{type} %{message}");
//    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& msg){
//        static QFile f("client.log"); static bool inited=false;
//        if(!inited){ f.open(QIODevice::Append|QIODevice::Text); inited=true; }
//        QTextStream ts(&f); ts << msg << '\n';  ts.flush();
//    });








    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/res/icon.ico"));
    QFile qss(":/style/stylesheet.qss");

    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
         qDebug("Open failed");
     }


    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                             QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://"+gate_host+":"+gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
