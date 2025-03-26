#include"global.h"


QString gate_url_prefix = "";


//    用来刷新qss
std::function<void(QWidget*)> repolish = [](QWidget* w) {
    // 取消 QWidget 的当前样式（即取消之前的样式设置）
    w->style()->unpolish(w);
    // 重新应用 QWidget 的样式
    w->style()->polish(w);
};



std::function<QString(QString)> xorString = [](QString input) {
    QString result = input; // 复制原始字符串，以便进行修改
    int length = input.length(); // 获取字符串的长度
    length = length % 255; // 取模，防止溢出

    for (int i = 0; i < input.length(); ++i) { // 遍历字符串的每个字符
        // 对每个字符进行异或操作
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(length)));
    }

    return result;
};
