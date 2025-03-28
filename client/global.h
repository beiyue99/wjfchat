#ifndef GLOBAL_H
#define GLOBAL_H
#include<QWidget>
#include<functional>
#include"QStyle"
#include<QRegularExpression>
#include<memory>
#include<iostream>
#include<mutex>
#include<QByteArray>
#include<QNetworkReply>
#include<QJsonObject>
#include<QDir>
#include<QSettings>



//    用来刷新qss
extern std::function<void(QWidget*)>repolish;

//  用来加密
extern std::function<QString(QString)> xorString;

//请求类型
enum ReqId{
    ID_GET_VARIFY_CODE=1001,//获取验证码
    ID_REG_USER=1002,//注册用户
    ID_RESET_PWD =1003,//重置密码
    ID_LOGIN_USER =1004,//用户登录
    ID_CHAT_LOGIN = 1005,//登陆聊天服务器
    ID_CHAT_LOGIN_RSP=1006, //登陆聊天服务器回包
};

//定义请求的模块
enum Modules{
    REGISTERMOD=0,  //注册模块
    RESETMOD = 1, //重置密码模块
    LOGINMOD = 2, //登录模块
};

//错误码
enum ErrorCodes{
    SUCCESS=0,
    ERR_JSON=1, //json解析失败
    ERR_NETWORK=2,//网络错误
};


// 注册输入时的错误码
enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_MISMATCH = 4,
    TIP_VERIFY_ERR = 5,
    TIP_USER_ERR = 6
};

// 输入密码小眼睛的选中状态
enum ClickLbState{
    Normal = 0,
    Selected = 1
};



struct ServerInfo{
    QString Host;  ///< 服务器主机地址
    QString Port;  ///< 服务器端口号
    QString Token; ///< 访问服务器的身份令牌
    int Uid;       ///< 用户 ID，用于标识当前用户
};

//访问gate_url的前缀
extern QString gate_url_prefix;


//聊天界面的几种模式
enum ChatUIMode{
    SearchMode,  //搜索模式
    ChatMode,  //聊天模式
    ContactMode,  //联系模式
};

//自定义QListwidgetItem的几种类型
enum ListItemType{
    CHAT_USER_ITEM,//聊天用户
    CONTACT_USER_ITEM,//联系人用户
    SEARCH_USER_ITEM,//搜索到的用户
    ADD_USER_TIP_ITEM,//提示添加用户
    INVALID_ITEM,//不可点击条目
    GROUP_TIP_ITEM,//分组提示条目
};

enum class ChatRole
{
    Self,
    Other
};

struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url,文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

//申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 40;
const QString add_prefix = "添加标签";
const int tip_offset = 5;


#endif // GLOBAL_H















