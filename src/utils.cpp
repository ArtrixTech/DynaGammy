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
#include "main.h"
#include "utils.h"

std::array<int, cfg_count> cfg =
{
    176,    // MinBr
    255,    // MaxBr
    70,     // Offset
    1,      // Temp
    3,      // Speed
    32,     // Threshold
    100,    // Polling_Rate
    1       // isAuto
};

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

        updateConfig();
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

void updateConfig()
{
#ifdef dbg
    std::cout << "Saving config... ";
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
        std::cout << "Unable to save settings file.\n";
        return;
    }

    for(size_t i = 0; i < cfg_count; i++)
    {
        std::string s = cfg_str[i];
        int val = cfg[i];

        std::string line (s + std::to_string(val));

        file << line << '\n';
    }

    file.close();
}

int calcBrightness(uint8_t* buf, uint64_t screen_res)
{
    uint64_t r_sum = 0;
    uint64_t g_sum = 0;
    uint64_t b_sum = 0;

    static uint64_t len = screen_res * 4; //4 bits per pixel

    for (uint64_t i = len - 4; i > 0; i -= 4)
    {
        r_sum += buf[i + 2];
        g_sum += buf[i + 1];
        b_sum += buf[i];
    }

    int luma = int((r_sum * 0.2126f + g_sum * 0.7152f + b_sum * 0.0722f) / screen_res);

#ifdef dbgluma
    std::cout << "\nRed: " << r_sum << '\n';
    std::cout << "Green: " << g_sum << '\n';
    std::cout << "Blue: " << b_sum << '\n';
    std::cout << "Luma:" << luma << '\n';
#endif

    return luma;
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
    std::cout << "Path: " << path << '\n';
#endif

    return path;
}
#else
void getGDISnapshot(uint8_t* buf, uint64_t len)
{
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, w, h);
    HDC memoryDC = CreateCompatibleDC(screenDC);

    HGDIOBJ oldObj = SelectObject(memoryDC, hBitmap);

    BitBlt(memoryDC, 0, 0, w, h, screenDC, 0, 0, SRCCOPY);

    BITMAPINFOHEADER bminfoheader;
    ::ZeroMemory(&bminfoheader, sizeof(BITMAPINFOHEADER));
    bminfoheader.biSize = sizeof(BITMAPINFOHEADER);
    bminfoheader.biWidth = w;
    bminfoheader.biHeight = -h;
    bminfoheader.biPlanes = 1;
    bminfoheader.biBitCount = 32;
    bminfoheader.biCompression = BI_RGB;
    bminfoheader.biSizeImage = len;
    bminfoheader.biClrUsed = 0;
    bminfoheader.biClrImportant = 0;

    GetDIBits(memoryDC, hBitmap, 0, h, buf, LPBITMAPINFO(&bminfoheader), DIB_RGB_COLORS);

    SelectObject(memoryDC, oldObj);
    DeleteObject(hBitmap);
    DeleteObject(oldObj);
    DeleteDC(memoryDC);
}

void setGDIBrightness(WORD brightness, int temp)
{

    if (brightness > default_brightness) {
        return;
    }

    WORD gammaArr[3][256];

    float gdiv = 1;
    float bdiv = 1;
    float val = temp;

    if(temp > 1)
    {
        bdiv += (val / 100);
        gdiv += (val / 270);
    }

    for (WORD i = 0; i < 256; ++i)
    {
        WORD gammaVal = i * brightness;

        gammaArr[0][i] = WORD (gammaVal);
        gammaArr[1][i] = WORD (gammaVal / gdiv);
        gammaArr[2][i] = WORD (gammaVal / bdiv);
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

    std::wstring appname = L"Gammy.exe";
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
