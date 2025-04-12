#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "friendlabel.h"
#include "userdata.h"

namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    // 好友申请界面，包括设置标签、显示更多标签、输入框文本处理、显示提示信息等。
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();

    // 初始化提示标签
    void InitTipLbs();

    // 添加提示标签
    void AddTipLbs(ClickedLabel* label, QPoint cur_point, QPoint &next_point, int text_width, int text_height);

    // 事件过滤器
    bool eventFilter(QObject *obj, QEvent *event) override;

    // 设置搜索信息
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);

private:
    // 重置标签状态
    void resetLabels();

    Ui::ApplyFriend *ui;

    // 存储已添加的标签
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;

    // 存储好友标签
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;

    // 添加新标签
    void addLabel(QString name);

    // 存储提示数据
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;

    // 存储搜索信息
    std::shared_ptr<SearchInfo> _si;

public slots:
    // 显示更多标签
    void ShowMoreLabel();

    // 输入标签后按回车键
    void SlotLabelEnter();

    // 移除好友标签
    void SlotRemoveFriendLabel(QString label);

    // 通过提示标签增删好友标签
    void SlotChangeFriendLabelByTip(QString label, ClickLbState state);

    // 监听输入框文本变化
    void SlotLabelTextChange(const QString& text);

    // 输入框输入完成
    void SlotLabelEditFinished();

    // 点击提示框内容添加好友标签
    void SlotAddFirendLabelByClickTip(QString text);

    // 确认申请好友
    void SlotApplySure();

    // 取消申请
    void SlotApplyCancel();
};

#endif // APPLYFRIEND_H
