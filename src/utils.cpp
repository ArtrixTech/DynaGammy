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
#include "cfg.h"

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
    LOGV << "Calculating brightness";
    uint64_t r{}, g{}, b{};

    static const uint64_t len = buf.size();

    // Remove the last 4 bits to avoid going out of bounds
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

// Map kelvin temperature to a step in the temperature array
int kelvinToStep(int temp)
{
    return ((max_temp_kelvin - temp) * temp_steps) / (max_temp_kelvin - min_temp_kelvin) + temp_mult;
}

#ifdef _WIN32

static const HDC screenDC = GetDC(nullptr);

void getGDISnapshot(std::vector<uint8_t> &buf)
{
    const static uint64_t w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN),
                          h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);

    static BITMAPINFOHEADER info;
    ZeroMemory(&info, sizeof(BITMAPINFOHEADER));
    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biWidth = w;
    info.biHeight = -h;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    info.biSizeImage = buf.size();
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, w, h);
    HDC memoryDC = CreateCompatibleDC(screenDC);

    HGDIOBJ oldObj = SelectObject(memoryDC, hBitmap);

    BitBlt(memoryDC, 0, 0, w, h, screenDC, 0, 0, SRCCOPY);

    GetDIBits(memoryDC, hBitmap, 0, h, buf.data(), LPBITMAPINFO(&info), DIB_RGB_COLORS);

    SelectObject(memoryDC, oldObj);
    DeleteObject(hBitmap);
    DeleteObject(oldObj);
    DeleteDC(memoryDC);
}

void setGDIGamma(int brightness, int temp)
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
        LOGI << "Gamma regkey already set";
        return;
    }

    LOGW << "Gamma regkey not set. Creating one...";

    HKEY hKey;

    s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

    if (s == ERROR_SUCCESS)
    {
        LOGI << "Gamma registry key created";

        DWORD val = 256;

        s = RegSetValueExW(hKey, L"GdiICMGammaRange", 0, REG_DWORD, LPBYTE(&val), sizeof(val));

        if (s == ERROR_SUCCESS)
        {
            MessageBoxW(nullptr, L"Gammy has extended the brightness range. Restart to apply the changes.", L"Gammy", 0);

            LOGI << "Gamma regvalue set";
        }
        else LOGE << "Error when setting Gamma registry value";
    }
    else
    {
        LOGE << "** Error when creating/opening gamma regkey";

        if (s == ERROR_ACCESS_DENIED)
        {
            LOGE << "** ACCESS_DENIED";
        }
    }

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
            s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, LPBYTE(path), int((wcslen(path) * sizeof(WCHAR))));

            if (s == ERROR_SUCCESS)
            {
                LOGI << "Startup regkey set";
            }
            else LOGE << "Failed to set startup regvalue";
        }
        else LOGE << "Failed to open startup regkey";
    }
    else
    {
        s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");

            if (s == ERROR_SUCCESS)
                LOGI << "Run at startup unset";
            else
                LOGE << "Failed to delete startup regkey";
    }

    if(hKey) RegCloseKey(hKey);
}

void checkInstance()
{
    LOGV << "Checking for multiple instances";

    HANDLE hStartEvent = CreateEventA(nullptr, true, false, "Gammy");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hStartEvent);
        hStartEvent = nullptr;

        LOGV << "Another instance of Gammy is running. Closing";
        exit(0);
    }
}
#endif
