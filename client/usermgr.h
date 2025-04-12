#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include <memory>
#include <singleton.h>
#include "userdata.h"
#include <vector>

// 用户管理类，继承自 QObject 并实现单例模式
class UserMgr : public QObject, public Singleton<UserMgr>, public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT

public:
    // 友元类，允许 Singleton 的友元访问 UserMgr 的构造函数
    friend class Singleton<UserMgr>;

    // 析构函数
    ~UserMgr();

    // 设置用户信息
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);

    // 设置用户的身份验证 token
    void SetToken(QString token);

    // 获取当前用户的 ID
    int GetUid();

    // 获取当前用户的名字
    QString GetName();

    // 获取当前用户的头像
    QString GetIcon();

    // 获取当前用户的详细信息
    std::shared_ptr<UserInfo> GetUserInfo();

    // 将申请列表追加到用户的申请列表中
    void AppendApplyList(QJsonArray array);

    // 将好友列表追加到用户的好友列表中
    void AppendFriendList(QJsonArray array);

    // 获取用户的申请列表
    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();

    // 向申请列表中添加一个新的申请
    void AddApplyList(std::shared_ptr<ApplyInfo> app);

    // 检查是否已经发起过对指定用户的申请
    bool AlreadyApply(int uid);

    // 获取每页显示的聊天列表
    std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage();

    // 判断聊天是否已经加载完成
    bool IsLoadChatFin();

    // 更新加载的聊天记录数量
    void UpdateChatLoadedCount();

    // 获取每页显示的联系人列表
    std::vector<std::shared_ptr<FriendInfo>> GetConListPerPage();

    // 更新加载的联系人数量
    void UpdateContactLoadedCount();

    // 判断联系人是否已经加载完成
    bool IsLoadConFin();

    // 通过 ID 检查是否为好友
    bool CheckFriendById(int uid);

    // 向好友列表中添加一个新的好友
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);

    // 向好友列表中添加一个新的好友（使用 AuthInfo）
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);

    // 通过 ID 获取指定的好友信息
    std::shared_ptr<FriendInfo> GetFriendById(int uid);

    // 向好友的聊天记录中追加新的聊天消息
    void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>>);

private:
    // 构造函数（私有，确保单例模式）
    UserMgr();

    // 当前用户信息
    std::shared_ptr<UserInfo> _user_info;

    // 申请列表
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;

    // 好友列表
    std::vector<std::shared_ptr<FriendInfo>> _friend_list;

    // 好友映射（通过好友ID查找）
    QMap<int, std::shared_ptr<FriendInfo>> _friend_map;

    // 身份验证 token
    QString _token;

    // 已加载的聊天记录数量
    int _chat_loaded;

    // 已加载的联系人数量
    int _contact_loaded;

public slots:
    // 添加好友响应处理槽函数
    void SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp);

    // 添加好友授权处理槽函数
    void SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth);
};

#endif // USERMGR_H
