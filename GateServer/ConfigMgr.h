#pragma once
#include "const.h"


struct SectionInfo {
    SectionInfo() {}
    ~SectionInfo() {
        _section_datas.clear();
    }

    SectionInfo(const SectionInfo& src) {
        _section_datas = src._section_datas;
    }

	//重载赋值运算符,用于将src的值赋给当前对象
    SectionInfo& operator = (const SectionInfo& src) {
        if (&src == this) {
            return *this;
        }
        this->_section_datas = src._section_datas;
        return *this;
    }

	std::map<std::string, std::string> _section_datas;  //存储key-value对的map
    std::string  operator[](const std::string& key) {
        if (_section_datas.find(key) == _section_datas.end()) {
            return "";
        }
        return _section_datas[key];
    }
};

class ConfigMgr
{
public:
    ~ConfigMgr() {
        _config_map.clear();
    }
	//重载中括号运算符,用于获取section对应的SectionInfo对象
    SectionInfo operator[](const std::string& section) {
        if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();  //如果没有找到则返回空的SectionInfo对象
        }
        return _config_map[section];
    }
    static ConfigMgr& Inst() {
        static ConfigMgr cfg_mgr;
        return cfg_mgr;
    }


    ConfigMgr& operator=(const ConfigMgr& src) {
        if (&src == this) {
            return *this;
        }
        this->_config_map = src._config_map;
    };

    ConfigMgr(const ConfigMgr& src) {
        this->_config_map = src._config_map;
    }


private:
    ConfigMgr();
    // 存储section和key-value对的map  
    std::map<std::string, SectionInfo> _config_map;
};