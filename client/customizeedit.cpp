#include "customizeedit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent):QLineEdit (parent),_max_len(0)
{
    connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::limitTextLength);
}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent *event)
{
    // 执行失去焦点时的自定义处理逻辑
    // 例如，可以在这里添加日志打印等
    // qDebug() << "CustomizeEdit focusout";

    // 调用基类 QLineEdit 的 focusOutEvent()，保持 QLineEdit 默认的行为
    QLineEdit::focusOutEvent(event);

    // 发送自定义信号，通知外部控件或类该控件失去焦点
    emit sig_foucus_out();
}

void CustomizeEdit::limitTextLength(QString text)
{
    if (_max_len <= 0) {
        return;
    }

    // 将文本转换为 UTF-8 编码的字节数组
    QByteArray byteArray = text.toUtf8();

    // 如果字节数组的大小超过了最大长度，截断到最大长度
    if (byteArray.size() > _max_len) {
        byteArray = byteArray.left(_max_len);
        // 更新文本内容为截断后的文本
        this->setText(QString::fromUtf8(byteArray));
    }
}
