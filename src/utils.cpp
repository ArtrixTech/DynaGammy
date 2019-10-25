/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "utils.h"

std::array<int, cfg_count> cfg
{
    192,    // MinBr
    255,    // MaxBr
    78,     // Offset
    0,      // Temp
    3,      // Speed
    32,     // Threshold
    100,    // Polling_Rate
    1       // isAuto
};

void setColors(int temp, std::array<double, 3> &c)
{
    size_t tick = size_t(temp / temp_mult);

    if(tick > temp_arr_entries) tick = temp_arr_entries;
    else if(tick < 1) tick = 1;

    size_t rpos = (temp_arr_entries - tick) * 3 + 0,
           gpos = (temp_arr_entries - tick) * 3 + 1,
           bpos = (temp_arr_entries - tick) * 3 + 2;

    c[0] = ingo_thies_table[rpos];
    c[1] = ingo_thies_table[gpos];
    c[2] = ingo_thies_table[bpos];
};

int calcBrightness(const std::vector<uint8_t> &buf)
{
    uint64_t r{}, g{}, b{};

    static const uint64_t len = buf.size();

    //Remove the last 4 bits to avoid going out of bounds
    for (auto i = len - 4; i > 0; i -= 4)
    {
        r += buf[i + 2];
        g += buf[i + 1];
        b += buf[i];
    }

    /*
     * The proper way would be to calculate perceived lightness as explained here: stackoverflow.com/a/56678483
     * But that's too heavy. We calculate luminance only, which still gives okay results.
     * Here it's converted to a 0-255 range by the RGB sums.
    */

    const static auto screen_res = len / 4;

    int brightness = (r * 0.2126 + g * 0.7152 + b * 0.0722) / screen_res;

    return brightness;
}

void readConfig()
{
#ifdef dbg
    std::cout << "Reading config...\n";
#endif

#ifdef _WIN32
    const std::wstring path = getExecutablePath(true);
#else
    const std::string path = getHomePath(true);
#endif

    std::fstream file(path, std::fstream::in | std::fstream::out | std::fstream::app);

    if(!file.is_open())
    {
        #ifdef dbg
        std::cout << "Unable to open settings file.\n";
        #endif
        return;
    }

    file.seekg(0, std::ios::end);
    bool empty = file.tellg() == 0;

    if(empty)
    {
        #ifdef dbg
        std::cout << "Config empty.\n";
        #endif

        saveConfig();
        return;
    }

    file.seekg(0);

    size_t c = 0;
    for (std::string line; std::getline(file, line);)
    {
        #ifdef dbgcfg
        std::cout << line << '\n';
        #endif

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
#ifdef dbg
    std::cout << "Saving config...\n";
#endif

#ifdef _WIN32
    static const std::wstring path = getExecutablePath(true);
#else
    static const std::string path = getHomePath(true);
#endif

    if(path.empty())
    {
#ifdef dbg
        std::cout << "Config path empty.\n";
#endif
        return;
    }

    std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);

    if(!file.good() || !file.is_open())
    {
#ifdef dbg
        std::cout << "Unable to open config file.\n";
#endif
        return;
    }

    for(size_t i = 0; i < cfg_count; i++)
    {
        std::string s = cfg_str[i];
        int val = cfg[i];

        std::string line (s + std::to_string(val));

        #ifdef dbgcfg
        std::cout << line << '\n';
        #endif

        file << line << '\n';
    }

    file.close();
}

#ifndef _WIN32
std::string getHomePath(bool add_cfg)
{
    const char* xdg_cfg_home;
    const char* home;

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

    if(add_cfg)
    {
        path += cfg_filename;
    }

#ifdef dbg
    std::cout << "Config path: " << path << '\n';
#endif

    return path;
}
#else

static const HDC screenDC = GetDC(nullptr);

void getGDISnapshot(uint8_t *buf, const uint64_t w, const uint64_t h)
{
    static uint64_t len = w * h * 4;

    static BITMAPINFOHEADER info;
    ZeroMemory(&info, sizeof(BITMAPINFOHEADER));
    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biWidth = w;
    info.biHeight = -h;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    info.biSizeImage = len;
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, w, h);
    HDC memoryDC = CreateCompatibleDC(screenDC);

    HGDIOBJ oldObj = SelectObject(memoryDC, hBitmap);

    BitBlt(memoryDC, 0, 0, w, h, screenDC, 0, 0, SRCCOPY);

    GetDIBits(memoryDC, hBitmap, 0, h, buf, LPBITMAPINFO(&info), DIB_RGB_COLORS);

    SelectObject(memoryDC, oldObj);
    DeleteObject(hBitmap);
    DeleteObject(oldObj);
    DeleteDC(memoryDC);
}

void setGDIGamma(WORD brightness, int temp)
{
    if (brightness > default_brightness) {
        return;
    }

    std::array c{1.0, 1.0, 1.0};

    setColors(temp, c);

    WORD gammaArr[3][256];

    for (WORD i = 0; i < 256; ++i)
    {
        WORD gammaVal = i * brightness;

        gammaArr[0][i] = WORD (gammaVal * c[0]);
        gammaArr[1][i] = WORD (gammaVal * c[1]);
        gammaArr[2][i] = WORD (gammaVal * c[2]);
    }

    SetDeviceGammaRamp(screenDC, gammaArr);
}

void checkGammaRange()
{
    LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM";
    LSTATUS s;

    s = RegGetValueW(HKEY_LOCAL_MACHINE, subKey, L"GdiICMGammaRange", RRF_RT_REG_DWORD, nullptr, nullptr, nullptr);

    if (s == ERROR_SUCCESS)
    {
#ifdef dbg
        std::cout << "Gamma registry key found.\n";
#endif
        return;
    }

#ifdef dbg
        std::cout << "Gamma registry key not found. Creating one...\n";
#endif

    HKEY hKey;

    s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

    if (s == ERROR_SUCCESS)
    {
#ifdef dbg
        std::cout << "Gamma registry key created.\n";
#endif
        DWORD val = 256;

        s = RegSetValueExW(hKey, L"GdiICMGammaRange", 0, REG_DWORD, LPBYTE(&val), sizeof(val));

        if (s == ERROR_SUCCESS)
        {
            MessageBoxW(nullptr, L"Gammy has extended the brightness range. Restart to apply the changes.", L"Gammy", 0);
#ifdef dbg
            std::cout << "Gamma registry value set.\n";
#endif
        }
#ifdef dbg
        else std::cout << "Error when setting Gamma registry value.\n";
#endif
    }
#ifdef dbg
    else
    {
        std::cout << "Error when creating/opening gamma RegKey.\n";

        if (s == ERROR_ACCESS_DENIED)
        {
            std::cout << "Access denied.\n";
        }
    }
#endif

    if (hKey) RegCloseKey(hKey);
}

void toggleRegkey(bool isChecked)
{
    LSTATUS s;
    HKEY hKey = nullptr;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (isChecked)
    {
        WCHAR path[MAX_PATH + 3], tmpPath[MAX_PATH + 3];
        GetModuleFileName(nullptr, tmpPath, MAX_PATH + 1);
        wsprintfW(path, L"\"%s\"", tmpPath);

        s = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);

        if (s == ERROR_SUCCESS)
        {
            #ifdef dbg
            std::cout << "RegKey opened.\n";
            #endif

            s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, LPBYTE(path), int((wcslen(path) * sizeof(WCHAR))));

            #ifdef dbg
                if (s == ERROR_SUCCESS) {
                    std::cout << "RegValue set.\n";
                }
                else {
                    std::cout << "Error when setting RegValue.\n";
                }
            #endif
        }
        #ifdef dbg
        else {
            std::cout << "Error when opening RegKey.\n";
        }
        #endif
    }
    else {
        s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");

        #ifdef dbg
            if (s == ERROR_SUCCESS)
                std::cout << "RegValue deleted.\n";
            else
                std::cout << "RegValue deletion failed.\n";
        #endif
    }

    if(hKey) RegCloseKey(hKey);
}

std::wstring getExecutablePath(bool add_cfg)
{
    wchar_t buf[FILENAME_MAX] {};
    GetModuleFileNameW(nullptr, buf, FILENAME_MAX);
    std::wstring path(buf);

    std::wstring appname = L"gammy.exe";
    path.erase(path.find(appname), appname.length());

    if(add_cfg) path += L"gammysettings.cfg";

    return path;
}

void checkInstance()
{
    HANDLE hStartEvent = CreateEventA(nullptr, true, false, "Gammy");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hStartEvent);
        hStartEvent = nullptr;
        exit(0);
    }
}
#endif
