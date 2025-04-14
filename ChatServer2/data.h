#pragma once
#include <string>
struct UserInfo {
	UserInfo():name(""), pwd(""),uid(0),email(""), icon(""), back("") {}
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string icon;
	std::string back;
};

struct ApplyInfo {
	ApplyInfo(int uid, std::string name,
		std::string icon,  int status)
		:_uid(uid),_name(name),
		_icon(icon),_status(status){}

	int _uid;
	std::string _name;
	std::string _icon;
	int _status;
};

