#include "userdata.h"
SearchInfo::SearchInfo(int uid, QString name, QString icon):_uid(uid)
  ,_name(name),_icon(icon){
}

AddFriendApply::AddFriendApply(int from_uid, QString name, QString icon)
    :_from_uid(from_uid),_name(name),_icon(icon)
{

}

void FriendInfo::AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData> > text_vec)
{
    for(const auto & text: text_vec){
      _chat_msgs.push_back(text);
    }
}
