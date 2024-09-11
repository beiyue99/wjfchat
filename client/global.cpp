#include"global.h"


//    用来刷新qss
std::function<void(QWidget*)> repolish = [](QWidget* w) {
    // 取消 QWidget 的当前样式（即取消之前的样式设置）
    w->style()->unpolish(w);
    // 重新应用 QWidget 的样式
    w->style()->polish(w);
};




QString gate_url_prefix = "";
