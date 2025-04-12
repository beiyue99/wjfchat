#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
#include <QDialog>
#include <memory>
#include "userdata.h"
#include "loadingdlg.h"

class SearchList : public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget *parent = nullptr); // 构造函数，初始化搜索结果列表

    void CloseFindDlg(); // 关闭查找对话框

    void SetSearchEdit(QWidget* edit); // 设置绑定的搜索框部件

protected:
    // 事件过滤器，控制滚动条的显示与滚动行为
    bool eventFilter(QObject *watched, QEvent *event) override ;

private:
    void waitPending(bool pending = true); // 设置是否处于等待响应状态

    void addTipItem(); // 添加提示项（如“搜索中...”）

    bool _send_pending; // 当前是否处于请求发送等待中
    std::shared_ptr<QDialog> _find_dlg; // 查找对话框指针
    QWidget* _search_edit; // 搜索输入框指针
    LoadingDlg* _loadingDialog; // 加载提示对话框

private slots:
    void slot_item_clicked(QListWidgetItem *item); // 当点击搜索结果项时触发

    void slot_user_search(std::shared_ptr<SearchInfo> si); // 当搜索到用户信息时触发

signals:
    void sig_jump_chat_item(std::shared_ptr<SearchInfo> si); // 发出跳转到对应聊天项的信号
};

#endif // SEARCHLIST_H
