#pragma once

#include "Log.h"
#include "Singleton.h"
#include <boost/filesystem.hpp>
#include <json/json.h>

struct SectionInfo
{
    std::string operator[](const std::string &key) const;
    std::map<std::string, std::string> _section_datas;
};

class ConfigMgr : public Singleton<ConfigMgr>
{
    friend class Singleton<ConfigMgr>;

public:
    ~ConfigMgr() override;

    SectionInfo operator[](const std::string &section) const;
    LogConfig getLogConfig() const;

private:
    ConfigMgr();
    void loadLogConfig();
    ConfigMgr(const ConfigMgr &src) = delete;
    ConfigMgr &operator=(const ConfigMgr &src) = delete;
    std::map<std::string, SectionInfo> _config_map;
    LogConfig _log_config;
};