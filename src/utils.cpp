/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
#include <Windows.h>
#endif

#include "utils.h"
#include "cfg.h"
#include "defs.h"

double lerp(double start, double end, double factor)
{
	return start * (1 - factor) + end * factor;
}

double normalize(double start, double end, double value)
{
	return (value - start) / (end - start);
}

double remap(double value, double from_min, double from_max, double to_min, double to_max)
{
	return lerp(to_min, to_max, normalize(from_min, from_max, value));
}

void setColors(int temp_step, std::array<double, 3> &c)
{
	const auto interpTemp = [temp_step] (size_t offset)
	{
		return remap(temp_slider_steps - temp_step, 0, temp_slider_steps, ingo_thies_table[offset], 1);
	};

	c[0] = interpTemp(0);
	c[1] = interpTemp(1);
	c[2] = interpTemp(2);
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

	int brightness = int((r * 0.2126 + g * 0.7152 + b * 0.0722) / screen_res);

	return brightness;
}

double easeOutExpo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

double easeInOutQuad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1) {
		return c / 2 * t * t + b;
	} else {
		return -c / 2 * ((--t) * (t - 2) - 1) + b;
	}
};

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
#include <algorithm>
void setGDIGamma(int brightness, int temp)
{
    if (brightness > brt_slider_steps) {
        return;
    }

    std::array c{1.0, 1.0, 1.0};

    setColors(temp, c);

    WORD ramp[3][256];

    for (WORD i = 0; i < 256; ++i)
    {
        const int val = remap(brightness, 0, brt_slider_steps, 0, 255) * i;

        ramp[0][i] = WORD(val * c[0]);
        ramp[1][i] = WORD(val * c[1]);
        ramp[2][i] = WORD(val * c[2]);
    }

    SetDeviceGammaRamp(screenDC, ramp);
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
