/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QApplication>
#include <QLabel>

#include "mainwindow.h"
#include "main.h"

#ifdef _WIN32
    #include <Windows.h>
    #include "DXGIDupl.h"

    #pragma comment(lib, "gdi32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "DXGI.lib")
    #pragma comment(lib, "D3D11.lib")
    #pragma comment(lib, "Advapi32.lib")
#elif __linux__
    #include "x11.h"
    #include <unistd.h>
#endif

#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <functional>

int	min_brightness   = 176;
int	max_brightness   = 255;
int	offset           = 70;
int	speed            = 3;
int	temp             = 1;
int	threshold        = 32;
int	polling_rate_ms  = 100;
int	polling_rate_min = 10;
int	polling_rate_max = 500;

int scrBr = default_brightness; //Current screen brightness

#ifdef _WIN32
DXGIDupl dx;
const HDC screenDC = GetDC(nullptr); //GDI Device Context of entire screen
const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
const int screenRes = w * h;
#elif __linux__
X11 x11;
const int screenRes = x11.getWidth() * x11.getHeight();
#endif

const unsigned bufLen = screenRes * 4;

int calcBrightness(uint8_t* buf)
{
    int r_sum = 0;
    int g_sum = 0;
    int b_sum = 0;

    for (int i = bufLen; i > 0; i -= 4)
    {
        r_sum += buf[i + 2];
        g_sum += buf[i + 1];
        b_sum += buf[i];
    }

    int luma = int((r_sum * 0.2126f + g_sum * 0.7152f + b_sum * 0.0722f)) / screenRes;

#ifdef dbg
    /*
    std::cout << "\nRed: " << r_sum << '\n';
    std::cout << "Green: " << g_sum << '\n';
    std::cout << "Blue: " << b_sum << '\n';
    std::cout << "Luma:" << luma << '\n';*/
#endif

    return luma;
}

#ifdef _WIN32
void getGDISnapshot(uint8_t* buf)
{
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, w, h);
    HDC memoryDC = CreateCompatibleDC(screenDC); //Memory DC for GDI screenshot

    HGDIOBJ oldObj = SelectObject(memoryDC, hBitmap);

    BitBlt(memoryDC, 0, 0, w, h, screenDC, 0, 0, SRCCOPY); //Slow, but works on Win7 and below

    //GetObject(hScreen, sizeof(screen), &screen);
    //int width(screen.bmWidth), height(screen.bmHeight);
    BITMAPINFOHEADER bminfoheader;
    ::ZeroMemory(&bminfoheader, sizeof(BITMAPINFOHEADER));
    bminfoheader.biSize = sizeof(BITMAPINFOHEADER);
    bminfoheader.biWidth = w;
    bminfoheader.biHeight = -h;
    bminfoheader.biPlanes = 1;
    bminfoheader.biBitCount = 32;
    bminfoheader.biCompression = BI_RGB;
    bminfoheader.biSizeImage = bufLen;
    bminfoheader.biClrUsed = 0;
    bminfoheader.biClrImportant = 0;

    GetDIBits(memoryDC, hBitmap, 0, h, buf, LPBITMAPINFO(&bminfoheader), DIB_RGB_COLORS);
    Sleep(polling_rate_ms);

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

    SetDeviceGammaRamp(screenDC, LPVOID(gammaArr));
}
#endif

struct Args {
    //Arguments to be passed to the AdjustBrightness thread
    short imgDelta = 0;
    short imgBr = 0;
    size_t threadCount = 0;
    MainWindow* w = nullptr;
};

void adjustBrightness(Args &args
                      #ifdef __linux__
                      ,
                      X11 &x11
                      #endif
                      )
{
    size_t threadId = ++args.threadCount;

    #ifdef dbgthread
        std::cout << "Thread " << threadId << " started...\n";
    #endif

    short sleeptime = (100 - args.imgDelta / 4) / speed;

    int targetBr = default_brightness - args.imgBr + offset;

    if		(targetBr > max_brightness) targetBr = max_brightness;
    else if (targetBr < min_brightness) targetBr = min_brightness;

    if (scrBr < targetBr) sleeptime /= 3;

    while (scrBr != targetBr && threadId == args.threadCount)
    {
        if (scrBr < targetBr)
        {
            ++scrBr;
        }
        else --scrBr;

#ifdef _WIN32
        setGDIBrightness(scrBr, temp);
#elif __linux__
        x11.setXF86Brightness(scrBr, temp);
#endif

        if(args.w->isVisible()) args.w->updateBrLabel();

        if (scrBr == min_brightness || scrBr == max_brightness)
        {
            targetBr = scrBr;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
    }

    #ifdef dbgthread
        std::printf("Thread %zd finished. Stopped: %d\n", threadId, threadId < args.threadCount);
    #endif
}

void app(MainWindow* wnd
         #ifdef _WIN32
         ,
         DXGIDupl &dx,
         const bool useDXGI
         #endif
         )
{
    #ifdef dbg
    std::cout << "Starting routine...\n";
    #endif

    int imgBr     = default_brightness;
    int old_imgBr = default_brightness;

    int old_min    = default_brightness;
    int old_max    = default_brightness;
    int old_offset = default_brightness;

    //Buffer to store screen pixels
    uint8_t* buf = nullptr;
    buf = new uint8_t[bufLen];

    Args args;
    args.w = wnd;

    short imgDelta = 0;
    bool forceChange = true;

#ifdef __linux__
    x11.setXF86Brightness(scrBr, temp);
#endif

    while (!wnd->quitClicked)
    {
#ifdef _WIN32
        if (useDXGI)
        {
            while (!dx.getDXGISnapshot(buf))
            {
                dx.restartDXGI();
            }
        }
        else
        {
            getGDISnapshot(buf);
        }
#elif __linux__
         x11.getX11Snapshot(buf);
#endif

        imgBr = calcBrightness(buf);
        //std::cout << imgBr << "\n";
        imgDelta += abs(old_imgBr - imgBr);

        if (imgDelta > threshold || forceChange)
        { 
            args.imgBr = short(imgBr);
            args.imgDelta = imgDelta;
            imgDelta = 0;

#ifdef _WIN32
            std::thread t(adjustBrightness, std::ref(args));
#elif __linux__
            std::thread t(adjustBrightness, std::ref(args), std::ref(x11));
#endif
            t.detach();

            forceChange = false;
        }

        if (min_brightness != old_min || max_brightness != old_max || offset != old_offset) {
            forceChange = true;
        }

        old_imgBr  = imgBr;
        old_min    = min_brightness;
        old_max    = max_brightness;
        old_offset = offset;
    }

#ifdef _WIN32
    setGDIBrightness(default_brightness, 1);
#else
    x11.setInitialGamma(false);
#endif
    delete[] buf;
    QApplication::quit();
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    #ifdef dbg
    FILE *f1, *f2, *f3;
    AllocConsole();
    freopen_s(&f1, "CONIN$", "r", stdin);
    freopen_s(&f2, "CONOUT$", "w", stdout);
    freopen_s(&f3, "CONOUT$", "w", stderr);

    #endif
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    checkInstance();

    const bool useDXGI = dx.initDXGI();

    if (!useDXGI)
    {
        polling_rate_min = 1000;
        polling_rate_max = 5000;
    }
    #endif

    readSettings();

#ifdef _WIN32
    checkGammaRange();
#endif
    QApplication a(argc, argv);
    MainWindow wnd;
    //wnd.show();

    #ifdef _WIN32
    std::thread t(app, &wnd, std::ref(dx), useDXGI);
    #else
    std::thread t(app, &wnd);
    #endif
    t.detach();

    return a.exec();
}

//////////////////////////////////////////////
#ifdef _WIN32
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
#endif

#ifdef _WIN32
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
#else
std::string getAppPath(bool add_cfg)
{
    #ifdef dbg
    std::cout << "Getting app path...\n";
    #endif

    char appPath[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", appPath, sizeof(appPath));
    if (len == -1 || len == sizeof(appPath)) len = 0;
    appPath[len] = '\0';

    std::string path(appPath);
    std::string appname = "Gammy";

    path.erase(path.find(appname), appname.length());

    if(add_cfg) path += settings_filename;

    return path;
}
#endif

void readSettings()
{
#ifdef _WIN32
    const std::wstring path = getExecutablePath(true);
#else
    const std::string path = getAppPath(true);
#endif

#ifdef dbg
    std::cout << "Opening file...\n";
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
        std::cout << "Settings file is empty. Writing defaults...\n";
        #endif

        std::array<std::string, settings_count> lines = {
            "minBrightness=" + std::to_string(min_brightness),
            "maxBrightness=" + std::to_string(max_brightness),
            "offset=" + std::to_string(offset),
            "speed=" + std::to_string(speed),
            "temp=" + std::to_string(temp),
            "threshold=" + std::to_string(threshold),
            "updateRate=" + std::to_string(polling_rate_ms)
        };

        for(const auto &s : lines) file << s << '\n';

        file.close();
        return;
    }

    //Read settings
    {
        std::array<int, settings_count> values;
        values.fill(-1);

        #ifdef dbg
        std::cout << "Reading settings...\n";
        #endif

        file.seekg(0);

        size_t c = 0;
        for (std::string line; std::getline(file, line);)
        {
            #ifdef dbg
            std::cout << line << '\n';
            #endif

            if(!line.empty()) values[c++] = std::stoi(line.substr(line.find('=') + 1));
        }

        if(values[0] == -1)
        {
            #ifdef dbg
                std::cout << "ERROR: unable to read settings.\n";
            #endif

            return;
        }

        min_brightness  = values[0];
        max_brightness  = values[1];
        offset          = values[2];
        speed           = values[3];
        temp            = values[4];
        threshold       = values[5];
        polling_rate_ms = values[6];

        if(polling_rate_ms < polling_rate_min) polling_rate_ms = polling_rate_min;
        if(polling_rate_ms > polling_rate_max) polling_rate_ms = polling_rate_max;
    }

    file.close();
}
