/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "main.h"
#include <QApplication>
#include <QLabel>

#ifdef _WIN32
    #include <Windows.h>
    #include <dxgi.h>
    #include <dxgi1_2.h>
    #include <d3d11.h>

    #pragma comment(lib, "gdi32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "DXGI.lib")
    #pragma comment(lib, "D3D11.lib")
    #pragma comment(lib, "Advapi32.lib")
#elif __linux__
    #include <unistd.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/xf86vmode.h>
    //#include <opencv2/opencv.hpp>
    //#include <opencv2/highgui.hpp>
    #include <cstdint>
    #include <cstring>
    #include <vector>
#endif

#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
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
const HDC screenDC = GetDC(nullptr); //GDI Device Context of entire screen

const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
#elif __linux__
Display* display = XOpenDisplay(nullptr);
const Screen* scr = DefaultScreenOfDisplay(display);

const int w = scr->width;
const int h = scr->height;
#endif

const int screenRes = w * h;
const unsigned bufLen = screenRes * 4;

#ifdef _WIN32
class DXGIDupl
{
    ID3D11Device*			d3d_device;
    ID3D11DeviceContext*	d3d_context;
    IDXGIOutput1*			output1;
    IDXGIOutputDuplication* duplication;
    D3D11_TEXTURE2D_DESC	tex_desc;

    public:
    bool initDXGI()
    {
        #ifdef dbg
        std::cout << "Initializing DXGI...\n";
        #endif

        IDXGIOutput* output;
        IDXGIAdapter1* pAdapter;
        std::vector<IDXGIOutput*> vOutputs; //Monitors vector
        std::vector<IDXGIAdapter1*> vAdapters; //GPUs vector

        HRESULT hr;

        //Retrieve a IDXGIFactory to enumerate the adapters
        {
            IDXGIFactory1* pFactory = nullptr;
            hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: failed to retrieve the IDXGIFactory.\n";
                getchar();
                #endif
                return false;
            }

            UINT i = 0;

            while (pFactory->EnumAdapters1(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                vAdapters.push_back(pAdapter);
            }

            pFactory->Release();

            #ifdef dbg
            //Get GPU info
            DXGI_ADAPTER_DESC1 desc;

            for (UINT i = 0; i < vAdapters.size(); ++i)
            {
                pAdapter = vAdapters[i];
                hr = pAdapter->GetDesc1(&desc);

                if (hr != S_OK)
                {
                    std::cout << "Error: failed to get a description for the adapter: " << i << '\n';
                    continue;
                }

                std::wprintf(L"Adapter %i: %lS\n", i, desc.Description);
            }
            #endif
        }

        //Get the monitors attached to the GPUs
        {
            UINT j;

            for (UINT i = 0; i < vAdapters.size(); ++i)
            {
                j = 0;
                pAdapter = vAdapters[i];
                while (pAdapter->EnumOutputs(j++, &output) != DXGI_ERROR_NOT_FOUND)
                {
                    #ifdef dbg
                    std::printf("Found monitor %d on adapter %d\n", j, i);
                    #endif
                    vOutputs.push_back(output);
                }
            }

            if (vOutputs.empty())
            {
                #ifdef dbg
                std::printf("Error: no outputs found (%zu).\n", vOutputs.size());
                getchar();
                #endif

                return false;
            }

            #ifdef dbg
                //Print monitor info.
                DXGI_OUTPUT_DESC desc;

                for (size_t i = 0; i < vOutputs.size(); ++i)
                {
                    output = vOutputs[i];
                    hr = output->GetDesc(&desc);

                    if (hr != S_OK)
                    {
                        printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %llu.\n", i);
                        continue;
                    }

                    std::wprintf(L"Monitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'y' : 'n');
                }
            #endif
        }

        //Create a Direct3D device to access the OutputDuplication interface
        {
            D3D_FEATURE_LEVEL d3d_feature_level;
            IDXGIAdapter1* d3d_adapter;
            UINT use_adapter = 0;

            if (vAdapters.size() <= use_adapter)
            {
                #ifdef dbg
                std::printf("Invalid adapter index: %d, we only have: %zu - 1\n", use_adapter, vAdapters.size());
                getchar();
                #endif

                return false;
            }

            d3d_adapter = vAdapters[use_adapter];

            if (!d3d_adapter)
            {
                #ifdef dbg
                std::cout << "Error: the stored adapter is nullptr.\n";
                getchar();
                #endif

                return false;
            }

            hr = D3D11CreateDevice(
                d3d_adapter,			  // Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter.
                D3D_DRIVER_TYPE_UNKNOWN,  // DriverType: We use the GPU as backing device.
                nullptr,                  // Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicable.
                NULL,                     // Flags: maybe we need to use D3D11_CREATE_DEVICE_BGRA_SUPPORT because desktop duplication is using this.
                nullptr,                  // Feature Levels:  what version to use.
                0,                        // Number of feature levels.
                D3D11_SDK_VERSION,        // The SDK version, use D3D11_SDK_VERSION
                &d3d_device,              // OUT: the ID3D11Device object.
                &d3d_feature_level,       // OUT: the selected feature level.
                &d3d_context);            // OUT: the ID3D11DeviceContext that represents the above features.

            d3d_context->Release();
            d3d_adapter->Release();

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: failed to create the D3D11 Device.\n";

                if (hr == E_INVALIDARG)
                {
                    std::cout << "Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?\n";
                }

                getchar();
                #endif

                return false;
            }
        }

        //Choose what monitor to use
        {
            UINT use_monitor = 0;
            output = vOutputs[use_monitor];

            if (vOutputs.size() <= use_monitor)
            {
                #ifdef dbg
                std::printf("Invalid monitor index: %d, we only have: %zu - 1\n", use_monitor, vOutputs.size());
                getchar();
                #endif

                return false;
            }

            if (!output)
            {
                #ifdef dbg
                std::cout << "No valid output found. The output is nullptr.\n";
                getchar();
                #endif

                return false;
            }
        }

        //Set texture properties
        {
            DXGI_OUTPUT_DESC output_desc;
            hr = output->GetDesc(&output_desc);

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: failed to get output description.\n";
                getchar();
                #endif

                return false;
            }

            tex_desc.Width = output_desc.DesktopCoordinates.right;
            tex_desc.Height = output_desc.DesktopCoordinates.bottom;
            tex_desc.MipLevels = 1;
            tex_desc.ArraySize = 1;
            tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            tex_desc.SampleDesc.Count = 1;
            tex_desc.SampleDesc.Quality = 0;
            tex_desc.Usage = D3D11_USAGE_STAGING;
            tex_desc.BindFlags = 0;
            tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            tex_desc.MiscFlags = 0;
        }

        //Initialize output duplication
        {
            hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1));

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: failed to query the IDXGIOutput1 interface.\n";
                getchar();
                #endif

                return false;
            }

            hr = output1->DuplicateOutput(d3d_device, &duplication);

            if (hr != S_OK)
            {
                #ifdef dbg
                std::cout << "Error: DuplicateOutput failed.\n";
                getchar();
                #endif

                return false;
            }

            output1->Release();
            d3d_device->Release();
        }

        for (auto adapter : vAdapters) adapter->Release();
        for (auto output : vOutputs) output->Release();

        return true;
    }

    bool getDXGISnapshot(uint8_t* buf) noexcept
    {
        HRESULT hr;

        ID3D11Texture2D* tex;
        DXGI_OUTDUPL_FRAME_INFO frame_info;
        IDXGIResource* desktop_resource;

        hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);

        switch(hr)
        {
            case S_OK:
            {
                //Get the texture interface
                hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));

                desktop_resource->Release();
                tex->Release();

                if (hr != S_OK)
                {
                    #ifdef dbg
                    std::cout << "Error: failed to query the ID3D11Texture2D interface on the IDXGIResource.\n";
                    #endif
                    return false;
                }

                break;
            }
            case DXGI_ERROR_ACCESS_LOST:
            {
                #ifdef dbg
                std::cout << "Received a DXGI_ERROR_ACCESS_LOST.\n";
                #endif
                return false;
            }
            case DXGI_ERROR_WAIT_TIMEOUT:
            {
                #ifdef dbg
                std::cout << "Received a DXGI_ERROR_WAIT_TIMEOUT.\n";
                #endif
                return false;
            }
            default:
            {
                #ifdef dbg
                std::cout << "Error: failed to acquire frame.\n";
                #endif
                return false;
            }
         };

        ID3D11Texture2D* staging_tex;
        hr = d3d_device->CreateTexture2D(&tex_desc, nullptr, &staging_tex);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Failed to create the 2D texture, error: " << hr << '\n';
            #endif
            return false;
        }

        d3d_context->CopyResource(staging_tex, tex);
        duplication->ReleaseFrame();

        D3D11_MAPPED_SUBRESOURCE map;

        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(polling_rate_ms));
        }
        while (d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map) == DXGI_ERROR_WAS_STILL_DRAWING);

        d3d_context->Unmap(staging_tex, 0);
        staging_tex->Release();
        d3d_context->Release();

        #ifdef dbg
        //std::cout << "depthPitch = " << map.DepthPitch << '\n';
        #endif

        //buf = reinterpret_cast<uint8_t*>(map.pData); //750 Ti: stable, Intel HD 2000: can crash when reading the buffer in calcBrightness
        memcpy(buf, map.pData, bufLen); //750 Ti: stable, Intel HD 2000: can cause a crash when copying

        return true;
    }

    void restartDXGI()
    {
        HRESULT hr;

        do {
            hr = output1->DuplicateOutput(d3d_device, &duplication);

            #ifdef dbg
            if (hr != S_OK)
            {
                std::cout << "Unable to duplicate output. Reason: ";

                switch (hr) {
                case E_INVALIDARG:	 printf("E_INVALIDARG\n"); break;
                case E_ACCESSDENIED: printf("E_ACCESSDENIED\n"); break;
                case DXGI_ERROR_UNSUPPORTED: printf("E_DXGI_ERROR_UNSUPPORTED\n"); break;
                case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: printf("DXGI_ERROR_NOT_CURRENTLY_AVAILABLE\n"); break;
                case DXGI_ERROR_SESSION_DISCONNECTED: printf("DXGI_ERROR_SESSION_DISCONNECTED\n"); break;
                }

                std::cout << "Retrying... (5 sec)\n";
            }
            #endif

            Sleep(5000);
        } while (hr != S_OK);

    #ifdef dbg
        std::cout << "Output duplication restarted successfully.\n";
    #endif

        output1->Release();
    }

    ~DXGIDupl()
    {
        if(duplication) duplication->ReleaseFrame();
        if(output1)     output1->Release();
        if(d3d_context) d3d_context->Release();
        if(d3d_device)  d3d_device->Release();
    }
};

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
#endif

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

#ifdef __linux__
class X11
{
private:
    int ramp_sz = 0;
    int screen = 0; //@TODO: actual screen number

    uint16_t* init_ramp;

    void fillRamp(uint16_t*& ramp, int amount)
    {
        uint16_t* r = &ramp[0 * ramp_sz];
        uint16_t* g = &ramp[1 * ramp_sz];
        uint16_t* b = &ramp[2 * ramp_sz];

        double slope = 1. * 32 / 255;

        uint16_t output = slope * amount;

#ifdef dbg
        std::cout << "ScrBr: " << amount << " Interp: " << output << '\n';
#endif

        for (uint16_t i = 0 ,val = 0; i < ramp_sz; i++)
        {
            val = i * output;

            r[i] = val;
            g[i] = val;
            b[i] = val;
        }
    }

public:
    X11()
    {
        #ifdef dbg
        std::cout << "Initializing XF86... ";
        #endif

        /**Query XF86Vidmode extension */
        {
            int ev_base, err_base;
            if (!XF86VidModeQueryExtension(display, &ev_base, &err_base))
            {
                #ifdef dbg
                std::cout << "Failed to query XF86VidMode extension.\n";
                #endif
                return;
            }

            int major_ver, minor_ver;
            if (!XF86VidModeQueryVersion(display, &major_ver, &minor_ver))
            {
                std::cout << "Failed to query XF86VidMode version.\n";
                return;
            }

            #ifdef dbg
            std::cout << "Major ver: " << major_ver << " Minor ver: " << minor_ver << "\n";
            #endif
       }

        /**Get initial gamma ramp and size */
        {
            if (!XF86VidModeGetGammaRampSize(display, screen, &ramp_sz))
            {
                #ifdef dbg
                std::cout << "Failed to get XF86 gamma ramp size.\n";
                #endif
                return;
            }

            init_ramp = new uint16_t[3 * ramp_sz * sizeof(uint16_t)];

            uint16_t* r = &init_ramp[0 * ramp_sz];
            uint16_t* g = &init_ramp[1 * ramp_sz];
            uint16_t* b = &init_ramp[2 * ramp_sz];

            if (!XF86VidModeGetGammaRamp(display, screen, ramp_sz, r, g, b))
            {
                #ifdef dbg
                std::cout << "Failed to get XF86 gamma ramp.\n";
                #endif
                return;
            }
        }
    }

    void getX11Snapshot(uint8_t* buf)
    {
        Window root;
        root = DefaultRootWindow(display);

        int x = 0, y = 0;
        XImage* img;

        img = XGetImage(display, root, x, y, w, h, AllPlanes, ZPixmap);
        std::this_thread::sleep_for(std::chrono::milliseconds(polling_rate_ms));
        memcpy(buf, img->data, bufLen);

        XDestroyImage(img);
    }

    void setXF86Brightness(uint16_t scrBr)
    {
        if (scrBr > default_brightness) {
            return;
        }

        uint16_t* ramp = new uint16_t[3 * ramp_sz * sizeof(uint16_t)];

        fillRamp(ramp, scrBr);

        delete[] ramp;

        XF86VidModeSetGammaRamp(display, screen, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);
    }

    void setInitialGamma()
    {
        XF86VidModeSetGammaRamp(display, screen, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);

        //uint16_t* ramp = new uint16_t[3 * ramp_sz * sizeof(uint16_t)];
        //fillRamp(ramp, max_brightness);
        //XF86VidModeSetGammaRamp(display, screen, ramp_sz, r, g, b);
    }

    ~X11()
    {
        XCloseDisplay(display);

        delete[] init_ramp;
    }
};
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
        x11.setXF86Brightness(scrBr);
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
    X11 x11;
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
    x11.setInitialGamma();
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

    DXGIDupl dx {};

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
    wnd.show();

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

    if(add_cfg) path += settings_filename;

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
