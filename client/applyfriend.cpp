#include "applyfriend.h"
#include "ui_applyfriend.h"
#include "clickedlabel.h"
#include "friendlabel.h"
#include <QScrollBar>
#include "usermgr.h"
#include "tcpmgr.h"

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplyFriend),_label_point(2,6)
{
    ui->setupUi(this);
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("ApplyFriend");
    this->setModal(true);
    ui->name_ed->setPlaceholderText(tr("wjf"));
    ui->lb_ed->setPlaceholderText("搜索、添加标签");
    ui->back_ed->setPlaceholderText("xxx");

	ui->lb_ed->SetMaxLength(21);
	ui->lb_ed->move(2, 2);
	ui->lb_ed->setFixedHeight(20);
	ui->lb_ed->setMaxLength(10);
	ui->input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);

	_tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
							 "父与子学Python","nodejs开发指南","go 语言开发指南",
								"游戏伙伴","金融投资","微信读书","拼多多拼友" };

    connect(ui->more_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::ShowMoreLabel);
    InitTipLbs();
    //链接输入标签回车事件
    connect(ui->lb_ed, &CustomizeEdit::returnPressed, this, &ApplyFriend::SlotLabelEnter);
    connect(ui->lb_ed, &CustomizeEdit::textChanged, this, &ApplyFriend::SlotLabelTextChange);
    connect(ui->lb_ed, &CustomizeEdit::editingFinished, this, &ApplyFriend::SlotLabelEditFinished);
    connect(ui->tip_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::SlotAddFirendLabelByClickTip);

    ui->scrollArea->horizontalScrollBar()->setHidden(true);
    ui->scrollArea->verticalScrollBar()->setHidden(true);
    ui->scrollArea->installEventFilter(this);
    ui->sure_btn->SetState("normal","hover","press");
    ui->cancel_btn->SetState("normal","hover","press");
    //连接确认和取消按钮的槽函数
    connect(ui->cancel_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(ui->sure_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend()
{
    delete ui;
}




//初始化并动态布局标签提示（Tip 标签），一般用于类似“推荐标签”或者“快捷标签”的 UI 场景，
//比如：你加好友时，系统推荐你选择“同学”“同事”“家人”等标签，点击即可快速填写
void ApplyFriend::InitTipLbs()
{
    int lines = 1;  // 当前显示的行数，初始为1

    for (int i = 0; i < _tip_data.size(); i++) {

        // 创建一个 ClickedLabel 标签，作为提示标签的一项
        auto* lb = new ClickedLabel(ui->lb_list);

        // 设置标签的多种状态样式（如普通、悬停、点击、选中等）
        lb->SetState("normal", "hover", "pressed", "selected_normal",
                     "selected_hover", "selected_pressed");

        lb->setObjectName("tipslb");           // 设置对象名称（方便样式表识别）
        lb->setText(_tip_data[i]);             // 设置标签文字
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip); // 绑定点击事件

        // 获取当前标签文字的宽高信息
        QFontMetrics fontMetrics(lb->font());         // 获取字体度量信息
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 计算文本宽度
        int textHeight = fontMetrics.height();         // 计算文本高度

        // 判断当前标签是否会超出一行最大宽度
        if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {
            lines++; // 新起一行
            if (lines > 2) { // 最多只显示两行提示标签
                delete lb;  // 超出行数限制就不添加了，释放内存
                return;
            }

            // 回到新行的起始位置
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15); // 换行偏移
        }

        // 保存当前位置为标签摆放起点
        auto next_point = _tip_cur_point;

        // 添加标签并设置它的位置与大小
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);

        // 更新下一个标签的起始位置
        _tip_cur_point = next_point;
    }
}


//将一个标签控件 lb 添加到标签列表区域并进行位置记录和管理
void ApplyFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    // 将标签移动到当前应该显示的位置
    lb->move(cur_point);

    // 显示这个标签
    lb->show();

    // 将标签对象插入到 _add_labels 字典中，key 是标签的文本内容，方便后续查找
    _add_labels.insert(lb->text(), lb);

    // 将标签文本内容保存到顺序列表中，记录添加顺序
    _add_label_keys.push_back(lb->text());

    // 计算下一个标签的位置（在当前标签右边留 15 像素空隙）
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y()); // y 坐标保持一致，不换行时标签在同一行
}


//当鼠标移入 scrollArea 时显示滚动条,当鼠标移出时隐藏滚动条
bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->scrollArea && event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}


// 存储搜索信息
void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si;
    auto applyname = UserMgr::GetInstance()->GetName();
    auto backname = si->_name;
    ui->name_ed->setText(applyname);
    ui->back_ed->setText(backname);
}

void ApplyFriend::ShowMoreLabel()
{
    ui->more_lb_wid->hide();

    ui->lb_list->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;
    //重拍现有的label
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font()); // 获取QLabel控件的字体信息
        textWidth = fontMetrics.horizontalAdvance(added_lb->text()); // 获取文本的宽度
        textHeight = fontMetrics.height(); // 获取文本的高度

        if(_tip_cur_point.x() +textWidth + tip_offset > ui->lb_list->width()){
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y()+textHeight+15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;

    }

    //添加未添加的
    for(int i = 0; i < _tip_data.size(); i++){
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end()){
            continue;
        }

		auto* lb = new ClickedLabel(ui->lb_list);
		lb->SetState("normal", "hover", "pressed", "selected_normal",
			"selected_hover", "selected_pressed");
		lb->setObjectName("tipslb");
		lb->setText(_tip_data[i]);
		connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

		QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
		int textHeight = fontMetrics.height(); // 获取文本的高度

		if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {

			_tip_cur_point.setX(tip_offset);
			_tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

		}

		 next_point = _tip_cur_point;

		AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);

		_tip_cur_point = next_point;

    }

   int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
   ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    //qDebug()<<"after resize ui->lb_list size is " <<  ui->lb_list->size();
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height()+diff_height);
}

void ApplyFriend::resetLabels()
{
    auto max_width = ui->gridWidget->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        //todo... 添加宽度统计
        if( _label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y()+iter.value()->height()+6);
            _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }

    if(_friend_labels.isEmpty()){
         ui->lb_ed->move(_label_point);
         return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()){
        ui->lb_ed->move(2,_label_point.y()+label_height+6);
    }else{
         ui->lb_ed->move(_label_point);
    }
}

void ApplyFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        ui->lb_ed->clear();
        return;
    }

	auto tmplabel = new FriendLabel(ui->gridWidget);
	tmplabel->SetText(name);
	tmplabel->setObjectName("FriendLabel");

	auto max_width = ui->gridWidget->width();
	//todo... 添加宽度统计
	if (_label_point.x() + tmplabel->width() > max_width) {
		_label_point.setY(_label_point.y() + tmplabel->height() + 6);
		_label_point.setX(2);
	}
	else {

	}


	tmplabel->move(_label_point);
	tmplabel->show();
	_friend_labels[tmplabel->Text()] = tmplabel;
	_friend_label_keys.push_back(tmplabel->Text());

	connect(tmplabel, &FriendLabel::sig_close, this, &ApplyFriend::SlotRemoveFriendLabel);

	_label_point.setX(_label_point.x() + tmplabel->width() + 2);

	if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()) {
		ui->lb_ed->move(2, _label_point.y() + tmplabel->height() + 2);
	}
	else {
		ui->lb_ed->move(_label_point);
	}

	ui->lb_ed->clear();

	if (ui->gridWidget->height() < _label_point.y() + tmplabel->height() + 2) {
		ui->gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
	}
}

void ApplyFriend::SlotLabelEnter()
{
    if(ui->lb_ed->text().isEmpty()){
        return;
    }

    auto text = ui->lb_ed->text();

    addLabel(ui->lb_ed->text());

    ui->input_tip_wid->hide();

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //找到了就只需设置状态为选中即可
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    //判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //标签展示栏也增加一个标签, 并设置绿色选中
    auto* lb = new ClickedLabel(ui->lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->lb_list->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{

    _label_point.setX(2);
    _label_point.setY(6);

   auto find_iter = _friend_labels.find(name);

   if(find_iter == _friend_labels.end()){
       return;
   }

   auto find_key = _friend_label_keys.end();
   for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end();
       iter++){
       if(*iter == name){
           find_key = iter;
           break;
       }
   }

   if(find_key != _friend_label_keys.end()){
      _friend_label_keys.erase(find_key);
   }


   delete find_iter.value();

   _friend_labels.erase(find_iter);

   resetLabels();

   auto find_add = _add_labels.find(name);
   if(find_add == _add_labels.end()){
        return;
   }

   find_add.value()->ResetNormalState();
}

//点击标已有签添加或删除新联系人的标签
void ApplyFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if(find_iter == _add_labels.end()){
        return;
    }

    if(state == ClickLbState::Selected){
        //编写添加逻辑
        addLabel(lbtext);
        return;
    }

    if(state == ClickLbState::Normal){
        //编写删除逻辑
        SlotRemoveFriendLabel(lbtext);
        return;
    }

}

void ApplyFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        ui->tip_lb->setText("");
        ui->input_tip_wid->hide();
        return;
    }

    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        ui->tip_lb->setText(new_text);
        ui->input_tip_wid->show();
        return;
    }
    ui->tip_lb->setText(text);
    ui->input_tip_wid->show();
}

void ApplyFriend::SlotLabelEditFinished()
{
    ui->input_tip_wid->hide();
}

void ApplyFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //找到了就只需设置状态为选中即可
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }
   
    //判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }
     
    //标签展示栏也增加一个标签, 并设置绿色选中
	auto* lb = new ClickedLabel(ui->lb_list);
	lb->SetState("normal", "hover", "pressed", "selected_normal",
		"selected_hover", "selected_pressed");
	lb->setObjectName("tipslb");
	lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
   
	QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度

	if (_tip_cur_point.x() + textWidth+ tip_offset+3 > ui->lb_list->width()) {

		_tip_cur_point.setX(5);
		_tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

	}

	auto next_point = _tip_cur_point;

	 AddTipLbs(lb, _tip_cur_point, next_point, textWidth,textHeight);
	_tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height()+ diff_height );
}

void ApplyFriend::SlotApplySure()
{
    // 构造要发送的 JSON 数据对象
    QJsonObject jsonObj;

    // 获取当前用户的 uid，并放入 JSON 中
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["uid"] = uid;

    // 获取用户填写的好友备注名称
    auto name = ui->name_ed->text();
    if (name.isEmpty()) {
        // 如果输入为空，则使用提示文本作为默认值
        name = ui->name_ed->placeholderText();
    }
    jsonObj["applyname"] = name;


    auto backname = ui->back_ed->text();
    if (backname.isEmpty()) {
        // 如果为空，也用提示文本作为默认内容
        backname = ui->back_ed->placeholderText();
    }
    jsonObj["back"] = backname;

    // 设置要添加的好友的 uid（由外部传入并提前记录在 _si 中）
    jsonObj["touid"] = _si->_uid;

    // 将 JSON 对象转为紧凑格式的 JSON 文本
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 通过 TCP 管理器发出添加好友请求
    emit TcpMgr::Inst()->sig_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);

    // 隐藏当前窗口
    this->hide();

    // 延迟删除该窗口对象，防止内存泄漏
    deleteLater();
}


void ApplyFriend::SlotApplyCancel()
{
    this->hide();
    deleteLater();
}

