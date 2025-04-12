#include "messagetextedit.h"
#include <QDebug>
#include <QMessageBox>

// 构造函数，初始化输入框属性
MessageTextEdit::MessageTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // 设置最大高度为60px，保持紧凑
    this->setMaximumHeight(60);

    // 暂未启用文本变化信号
    // connect(this, SIGNAL(textChanged()), this, SLOT(textEditChanged()));
}

// 析构函数，无特别清理操作
MessageTextEdit::~MessageTextEdit()
{
}

// 获取当前用户输入的消息内容（可能包含文本、图片、文件）
QVector<MsgInfo> MessageTextEdit::getMsgList()
{
    mGetMsgList.clear();

    QString doc = this->document()->toPlainText(); // 获取纯文本
    QString text = ""; // 临时保存普通文本
    int indexUrl = 0;
    int count = mMsgList.size();

    for (int index = 0; index < doc.size(); index++) {
        if (doc[index] == QChar::ObjectReplacementCharacter) {
            // 发现占位符（表示图片或文件），先保存前面的纯文本
            if (!text.isEmpty()) {
                QPixmap pix;
                insertMsgList(mGetMsgList, "text", text, pix);
                text.clear();
            }

            // 把当前位置的特殊内容加入消息列表
            while (indexUrl < count) {
                MsgInfo msg = mMsgList[indexUrl];
                if (this->document()->toHtml().contains(msg.content, Qt::CaseSensitive)) {
                    indexUrl++;
                    mGetMsgList.append(msg);
                    break;
                }
                indexUrl++;
            }
        } else {
            text.append(doc[index]);
        }
    }

    // 最后如果还有纯文本也加进去
    if (!text.isEmpty()) {
        QPixmap pix;
        insertMsgList(mGetMsgList, "text", text, pix);
        text.clear();
    }

    mMsgList.clear(); // 清空历史记录
    this->clear();    // 清空编辑框
    return mGetMsgList;
}

// 拖拽进入时判断是否允许
void MessageTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->source() == this)
        event->ignore(); // 拒绝自己拖自己
    else
        event->accept();
}

// 拖拽释放时处理插入
void MessageTextEdit::dropEvent(QDropEvent *event)
{
    insertFromMimeData(event->mimeData());
    event->accept();
}

// 监听键盘按下事件，按下 Enter 发消息，Shift+Enter 换行
void MessageTextEdit::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
        && !(e->modifiers() & Qt::ShiftModifier))
    {
        emit send(); // 发送消息信号
        return;
    }

    // 其他按键交给原始处理
    QTextEdit::keyPressEvent(e);
}

// 插入拖拽的文件列表
void MessageTextEdit::insertFileFromUrl(const QStringList &urls)
{
    if (urls.isEmpty())
        return;

    for (QString url : urls) {
        if (isImage(url))
            insertImages(url);
        else
            insertTextFile(url);
    }
}

// 插入图片内容并显示
void MessageTextEdit::insertImages(const QString &url)
{
    QImage image(url);

    // 控制图片最大尺寸，避免太大
    if (image.width() > 120 || image.height() > 80) {
        if (image.width() > image.height())
            image = image.scaledToWidth(120, Qt::SmoothTransformation);
        else
            image = image.scaledToHeight(80, Qt::SmoothTransformation);
    }

    // 插入图片到光标位置
    QTextCursor cursor = this->textCursor();
    cursor.insertImage(image, url);

    // 保存图片消息记录
    insertMsgList(mMsgList, "image", url, QPixmap::fromImage(image));
}

// 插入文本文件图标（不显示内容，只显示文件名和大小）
void MessageTextEdit::insertTextFile(const QString &url)
{
    QFileInfo fileInfo(url);

    // 拖拽的是文件夹
    if (fileInfo.isDir()) {
        QMessageBox::information(this, "提示", "只允许拖拽单个文件!");
        return;
    }

    // 超过 100MB 文件不支持
    if (fileInfo.size() > 100 * 1024 * 1024) {
        QMessageBox::information(this, "提示", "发送的文件大小不能大于100M");
        return;
    }

    QPixmap pix = getFileIconPixmap(url); // 获取带有文件名和大小的图标
    QTextCursor cursor = this->textCursor();
    cursor.insertImage(pix.toImage(), url);
    insertMsgList(mMsgList, "file", url, pix);
}

// 判断是否支持 mime 类型插入
bool MessageTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return QTextEdit::canInsertFromMimeData(source);
}

// 处理粘贴或拖拽的 mime 数据
void MessageTextEdit::insertFromMimeData(const QMimeData *source)
{
    QStringList urls = getUrl(source->text());
    if (urls.isEmpty())
        return;

    for (QString url : urls) {
        if (isImage(url))
            insertImages(url);
        else
            insertTextFile(url);
    }
}

// 判断文件是否为图片类型
bool MessageTextEdit::isImage(QString url)
{
    QString imageFormat = "bmp,jpg,png,tif,gif,pcx,tga,exif,fpx,svg,psd,cdr,pcd,dxf,ufo,eps,ai,raw,wmf,webp";
    QStringList imageFormatList = imageFormat.split(",");
    QFileInfo fileInfo(url);
    QString suffix = fileInfo.suffix();

    return imageFormatList.contains(suffix, Qt::CaseInsensitive);
}

// 将消息内容添加到消息列表中
void MessageTextEdit::insertMsgList(QVector<MsgInfo> &list, QString flag, QString text, QPixmap pix)
{
    MsgInfo msg;
    msg.msgFlag = flag;
    msg.content = text;
    msg.pixmap = pix;
    list.append(msg);
}

// 从富文本提取出 file:// 路径
QStringList MessageTextEdit::getUrl(QString text)
{
    QStringList urls;
    if (text.isEmpty()) return urls;

    QStringList list = text.split("\n");
    for (QString url : list) {
        if (!url.isEmpty()) {
            QStringList str = url.split("///");
            if (str.size() >= 2)
                urls.append(str.at(1));
        }
    }
    return urls;
}

// 根据文件路径生成一个带图标、文件名、文件大小的 QPixmap，用于展示在文本框里
QPixmap MessageTextEdit::getFileIconPixmap(const QString &url)
{
    QFileIconProvider provider;
    QFileInfo fileinfo(url);
    QIcon icon = provider.icon(fileinfo);

    QString strFileSize = getFileSize(fileinfo.size());

    QFont font(QString("宋体"), 10, QFont::Normal, false);
    QFontMetrics fontMetrics(font);
    QSize textSize = fontMetrics.size(Qt::TextSingleLine, fileinfo.fileName());
    QSize fileSize = fontMetrics.size(Qt::TextSingleLine, strFileSize);

    int maxWidth = qMax(textSize.width(), fileSize.width());
    QPixmap pix(50 + maxWidth + 10, 50);
    pix.fill();

    QPainter painter;
    painter.begin(&pix);
    painter.drawPixmap(QRect(0, 0, 50, 50), icon.pixmap(40, 40));
    painter.setPen(Qt::black);
    painter.drawText(QRect(60, 3, textSize.width(), textSize.height()), fileinfo.fileName());
    painter.drawText(QRect(60, textSize.height() + 5, fileSize.width(), fileSize.height()), strFileSize);
    painter.end();

    return pix;
}

// 将字节大小格式化为字符串（B/KB/MB/GB）
QString MessageTextEdit::getFileSize(qint64 size)
{
    QString Unit;
    double num;
    if (size < 1024) {
        num = size;
        Unit = "B";
    } else if (size < 1024 * 1024) {
        num = size / 1024.0;
        Unit = "KB";
    } else if (size < 1024 * 1024 * 1024) {
        num = size / 1024.0 / 1024.0;
        Unit = "MB";
    } else {
        num = size / 1024.0 / 1024.0 / 1024.0;
        Unit = "GB";
    }
    return QString::number(num, 'f', 2) + " " + Unit;
}

// 监听文本变化（未启用）
void MessageTextEdit::textEditChanged()
{
    // qDebug() << "text changed!" << endl;
}
