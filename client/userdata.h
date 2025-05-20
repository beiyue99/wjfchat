#ifndef USERDATA_H
#define USERDATA_H
#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
#include <QDebug>

//存储用户信息
class SearchInfo {
public:
    SearchInfo(int uid, QString name,  QString icon);
	int _uid;
    QString _name;
    QString _icon;
};

//存储添加好友申请的相关信息
class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString icon);
	int _from_uid;
    QString _name;
    QString _icon;
};

//保存好友申请的详细信息,包括处理状态
struct ApplyInfo {
    ApplyInfo(int uid, QString name,QString icon, int status)
        :_uid(uid),_name(name),
        _icon(icon),_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :_uid(addinfo->_from_uid),_name(addinfo->_name),_icon(addinfo->_icon),
          _status(0)
    {}
    void SetIcon(QString head){
        _icon = head;
    }
    int _uid;
    QString _name;
    QString _icon;
    int _status;
};

// 存储用户的基本信息
struct AuthInfo {
    AuthInfo(int uid, QString name, QString icon):
        _uid(uid), _name(name),  _icon(icon){}

    int _uid;       // 用户的唯一ID
    QString _name;  // 用户的用户名
    QString _icon;  // 用户的头像
};

// 存储认证响应信息
struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name, QString peer_icon)
        :_uid(peer_uid),_name(peer_name),_icon(peer_icon)
    {}

    int _uid;       // 对方用户的唯一ID
    QString _name;  // 对方用户的用户名
    QString _icon;  // 对方用户的头像
};

// 存储聊天相关信息
struct TextChatData {
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid)
        :_msg_id(msg_id),_msg_content(msg_content),_from_uid(fromuid),_to_uid(touid){}

    QString _msg_id;       // 消息的唯一ID
    QString _msg_content;  // 消息内容
    int _from_uid;         // 发送者的用户ID
    int _to_uid;           // 接收者的用户ID
};

// 存储好友信息，包括用户信息、描述、背景信息等
struct FriendInfo {
    FriendInfo(int uid, QString name,QString icon, QString back, QString last_msg="")
        :_uid(uid), _name(name),_icon(icon),_back(back),_last_msg(last_msg){}

    // 从 AuthInfo 构造 FriendInfo
    FriendInfo(std::shared_ptr<AuthInfo> auth_info):_uid(auth_info->_uid),_icon(auth_info->_icon),_name(auth_info->_name){}

    // 从 AuthRsp 构造 FriendInfo
    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):_uid(auth_rsp->_uid),_icon(auth_rsp->_icon),_name(auth_rsp->_name){}

    // 追加聊天记录
    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec);

    int _uid;               // 好友的唯一ID
    QString _name;          // 好友的用户名
    QString _icon;          // 好友的头像
    QString _back;
    QString _last_msg;      // 好友的最后一条消息
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;  // 好友的聊天记录
};

// 存储用户的详细信息，包括聊天记录等
struct UserInfo {
    UserInfo(int uid, QString name,  QString icon, QString last_msg = "")
        :_uid(uid),_name(name),_icon(icon),_last_msg(last_msg){}

    // 从 AuthInfo 构造 UserInfo
    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid),_name(auth->_name),_icon(auth->_icon),_last_msg(""){}

    // 从 AuthRsp 构造 UserInfo
    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid),_name(auth->_name),
        _icon(auth->_icon),_last_msg(""){}

    // 从 SearchInfo 构造 UserInfo
    UserInfo(std::shared_ptr<SearchInfo> search_info):
        _uid(search_info->_uid),_name(search_info->_name),
        _icon(search_info->_icon),_last_msg(""){}

    // 从 FriendInfo 构造 UserInfo
    UserInfo(std::shared_ptr<FriendInfo>& friend_info):
        _uid(friend_info->_uid),_name(friend_info->_name),
        _icon(friend_info->_icon),_last_msg("") {
        if (!friend_info) {                                  // ← 保护
            qDebug() << "www";
            _uid = -1;
            return;                                // 或者 throw/默认值
        }
        _chat_msgs = friend_info->_chat_msgs;  // 保存好友的聊天记录
    }

    int _uid;               // 用户的唯一ID
    QString _name;          // 用户的用户名
    QString _icon;          // 用户的头像
    QString _last_msg;      // 用户的最后一条消息
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;  // 用户的聊天记录
};

// 存储消息的详细信息
struct TextChatMsg {
    TextChatMsg(int fromuid, int touid, QJsonArray arrays)
        :_from_uid(fromuid),_to_uid(touid){
        // 将消息数组解析为文本聊天数据
        for(auto  msg_data : arrays){
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content, fromuid, touid);
            _chat_msgs.push_back(msg_ptr);
        }
    }

    int _to_uid;               // 接收者的用户ID
    int _from_uid;             // 发送者的用户ID
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;  // 消息列表
};


#endif
