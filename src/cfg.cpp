#include "cfg.h"
#include "utils.h"
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <Windows.h> // GetModuleFileNameW
#endif

std::array<int, cfg_count> cfg
{
    192,    // MinBr
    255,    // MaxBr
    78,     // Offset
    0,      // Temp
    3,      // Speed
    32,     // Threshold
    100,    // Polling_Rate
    1,      // isAuto
    1       // Debug
};

void readConfig()
{
#ifdef _WIN32
    const std::wstring path = getExecutablePath();
#else
    const std::string path = getHomePath();
#endif

    LOGI << "Opening config";
    std::fstream file(path, std::fstream::in | std::fstream::out | std::fstream::app);

    if(!file.is_open())
    {
        LOGE << "Unable to open config file";

        return;
    }

    file.seekg(0, std::ios::end);
    bool empty = file.tellg() == 0;

    if(empty)
    {
        LOGW << "Config empty";

        saveConfig();
        return;
    }

    file.seekg(0);

    size_t c = 0;
    for (std::string line; std::getline(file, line);)
    {
        LOGV << line;

        if(!line.empty())
        {
            size_t pos = line.find('=') + 1;
            std::string val = line.substr(pos);

            cfg[c++] = std::stoi(val);
        }
    }

    file.close();
}

void saveConfig()
{
#ifdef _WIN32
    static const std::wstring path = getExecutablePath();
#else
    static const std::string path = getHomePath();
#endif

    if(path.empty())
    {
        LOGE << "Config path empty";
        return;
    }

    std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);

    if(!file.good() || !file.is_open())
    {
        LOGE << "Unable to open config file";
        return;
    }

    for(size_t i = 0; i < cfg_count; i++)
    {
        std::string s = cfg_str[i];

        int val = cfg[i];

        std::string line (s + std::to_string(val));

        LOGV << line;

        file << line << '\n';
    }

    file.close();
}

#ifndef _WIN32
std::string getHomePath()
{
    const char *xdg_cfg_home;
    const char *home;

    std::string path;
    std::string cfg_filename = "gammy";

    xdg_cfg_home = getenv("XDG_CONFIG_HOME");

    if (xdg_cfg_home)
    {
       path = std::string(xdg_cfg_home) + '/';
    }
    else
    {
        home = getenv("HOME");
        if(!home) return "";

        path = std::string(home) + "/.config/";
    }

    path += cfg_filename;

    LOGI << "Config path: " + path;

    return path;
}
#else
std::wstring getExecutablePath()
{
    wchar_t buf[FILENAME_MAX] {};
    GetModuleFileNameW(nullptr, buf, FILENAME_MAX);
    std::wstring path(buf);

    std::wstring appname = L"gammy.exe";
    path.erase(path.find(appname), appname.length());

    path += L"gammysettings.cfg";

    return path;
}
#endif


