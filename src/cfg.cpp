/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

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
    0,      // toggleLimit
    3,      // Debug (0: none, 1: fatal, 2: error, 3: warning, 4: info, 5: debug, 6: verbose)
    255     // CurBr
};

void readConfig()
{
#ifdef _WIN32
    static const std::wstring path = getExecutablePath();
#else
    static const std::string path = getHomePath();
#endif

    LOGD << "Opening config...";

    std::fstream file(path, std::fstream::in | std::fstream::out | std::fstream::app);

    if(!file.is_open())
    {
        LOGE << "Unable to open config file";

        return;
    }

    LOGD << "Config opened";

    file.seekg(0, std::ios::end);
    bool empty = file.tellg() == 0;

    if(empty)
    {
        LOGW << "Config empty. Creating one...";

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
        LOGE << "Invalid config path";
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
        file << cfg_str[i] << cfg[i] << '\n';
    }

    LOGI << "Settings saved";

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


