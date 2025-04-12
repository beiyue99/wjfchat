#ifndef LOADINGDLG_H
#define LOADINGDLG_H

#include <QDialog>

namespace Ui {
class LoadingDlg;
}

// 加载中对话框，用于显示动画效果提示用户正在加载
class LoadingDlg : public QDialog
{
    Q_OBJECT

public:
    // 构造函数，初始化加载动画界面
    explicit LoadingDlg(QWidget *parent = nullptr);

    // 析构函数，释放资源
    ~LoadingDlg();

private:
    Ui::LoadingDlg *ui; // UI界面指针
};

#endif // LOADINGDLG_H
