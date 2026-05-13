#pragma once

#include "Singleton.h"
#include <boost/filesystem.hpp>
#include <json/json.h>

struct SectionInfo
{
    SectionInfo();
    ~SectionInfo();
    SectionInfo(const SectionInfo &src);
    SectionInfo &operator=(const SectionInfo &src);

    std::string operator[](const std::string &key);
    std::map<std::string, std::string> _section_datas;
};

class ConfigMgr : public Singleton<ConfigMgr>
{
    friend class Singleton<ConfigMgr>;

public:
    ~ConfigMgr() override;

    SectionInfo operator[](const std::string &section);

private:
    ConfigMgr();
    ConfigMgr(const ConfigMgr &src) = delete;
    ConfigMgr &operator=(const ConfigMgr &src) = delete;
    std::map<std::string, SectionInfo> _config_map;
};