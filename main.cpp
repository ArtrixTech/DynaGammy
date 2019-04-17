/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "main.h"
#include <QApplication>
#include <QLabel>
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include <functional>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Advapi32.lib")

unsigned char	MIN_BRIGHTNESS  = 176;
unsigned char	MAX_BRIGHTNESS  = 255;
unsigned short	OFFSET          = 70;
unsigned char	SPEED           = 2;
unsigned char	TEMP            = 1;
unsigned char	THRESHOLD       = 32;
unsigned short	UPDATE_TIME_MS  = 50;
unsigned short	UPDATE_TIME_MIN;
unsigned short	UPDATE_TIME_MAX;

unsigned short scrBr       = DEFAULT_BRIGHTNESS;  //Current screen brightness
unsigned short targetScrBr = DEFAULT_BRIGHTNESS;  //Difference between max and current screen brightness

HDC screenDC = GetDC(nullptr); //GDI Device Context of entire screen

int w, h, bufLen;
int calcBufLen(int &w, int &h);

unsigned short getBrightness(unsigned char* buf)
{
    UCHAR r = buf[2];
    UCHAR g = buf[1];
    UCHAR b = buf[0];

    int colorSum = 0;

    for (auto i = bufLen; i > 0; i -= 4)
    {
        r = buf[i + 2];
        g = buf[i + 1];
        b = buf[i];

        colorSum += (r + g + b) / 3;
    }

    return colorSum / (w * h); //Assigns a value between 0 and 255
}

struct DXGIDupl
{
    private:
    ID3D11Device*			d3d_device;
    ID3D11DeviceContext*	d3d_context;
    IDXGIOutput1*			output1;
    IDXGIOutputDuplication* duplication;
    D3D11_TEXTURE2D_DESC	tex_desc;

    public:
    bool initDXGI()
    {
        #ifdef dbg
        std::cout << "Initializing DXGI..." << std::endl;
        #endif

        HRESULT hr;
        IDXGIFactory1* pFactory;

        //Retrieve a IDXGIFactory to enumerate the adapters
        hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to retrieve the IDXGIFactory.\n" << std::endl;
            getchar();
            #endif
            return false;
        }

        IDXGIAdapter1* pAdapter;
        std::vector<IDXGIAdapter1*> vAdapters; //GPUs vector
        std::vector<IDXGIOutput*>   vOutputs; //Monitors vector

        UINT i = 0;

        while (pFactory->EnumAdapters1(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            vAdapters.push_back(pAdapter);
        }

        pFactory->Release();

        #ifdef dbg
        //Get GPU info
        for (UINT i = 0; i < vAdapters.size(); ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            pAdapter = vAdapters[i];
            hr = pAdapter->GetDesc1(&desc);

            if (hr != S_OK)
            {
                std::cout << "Error: failed to get a description for the adapter: " << i << std::endl;
                continue;
            }

            std::wprintf(L"Adapter %i: %lS\n", i, desc.Description);
        }
        #endif

        //Get the monitors attached to the GPUs. We don't use IDXGIOutput1 because it lacks EnumOuputs.
        IDXGIOutput* output;

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

        if (vOutputs.size() <= 0)
        {
            #ifdef dbg
            std::printf("Error: no outputs found (%zu).\n", vOutputs.size());
            getchar();
            #endif

            return false;
        }

        #ifdef dbg
        //Print monitor info.
        for (size_t i = 0; i < vOutputs.size(); ++i)
        {
            DXGI_OUTPUT_DESC desc;
            output = vOutputs[i];
            hr = output->GetDesc(&desc);

            if (hr != S_OK) {
                printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %llu.\n", i);
                continue;
            }

            std::wprintf(L"Monitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'y' : 'n');
        }
        #endif

        //Create a Direct3D device to access the OutputDuplication interface
        IDXGIAdapter1* d3d_adapter;
        D3D_FEATURE_LEVEL d3d_feature_level;

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
            std::cout << "Error: the stored adapter is nullptr." << std::endl;
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
            std::cout << "Error: failed to create the D3D11 Device." << std::endl;

            if (hr == E_INVALIDARG)
            {
                std::cout << "Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?" << std::endl;
            }

            getchar();
            #endif

            return false;
        }

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
            std::cout << "No valid output found. The output is nullptr." << std::endl;
            getchar();
            #endif

            return false;
        }

        hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to query the IDXGIOutput1 interface." << std::endl;
            getchar();
            #endif

            return false;
        }

        DXGI_OUTPUT_DESC output_desc;
        hr = output->GetDesc(&output_desc);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: failed to get output description." << std::endl;
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

        hr = output1->DuplicateOutput(d3d_device, &duplication);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::cout << "Error: DuplicateOutput failed." << std::endl;
            getchar();
            #endif

            return false;
        }

        output1->Release();
        d3d_device->Release();

        for (auto adapter : vAdapters) adapter->Release();
        for (auto output : vOutputs) output->Release();

        return true;
    }

    bool getDXGISnapshot(unsigned char* &buf)
    {
        HRESULT hr;

        ID3D11Texture2D* tex;
        DXGI_OUTDUPL_FRAME_INFO frame_info;
        IDXGIResource* desktop_resource;

        if (!duplication) {
            #ifdef dbg
            std::cout << "Duplication is nullptr." <<std::endl;
            #endif
            return false;
        }

        hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);

        switch(hr)
        {
            case S_OK:
            {
                //Get the texture interface
                hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

                desktop_resource->Release();
                tex->Release();

                if (hr != S_OK)
                {
                    #ifdef dbg
                    std::cout << "Error: failed to query the ID3D11Texture2D interface on the IDXGIResource." << std::endl;
                    #endif
                    return false;
                }

                break;
            }
            case DXGI_ERROR_ACCESS_LOST:
            {
                #ifdef dbg
                std::cout << "Received a DXGI_ERROR_ACCESS_LOST." << std::endl;
                #endif
                return false;
            }
            case DXGI_ERROR_WAIT_TIMEOUT:
            {
                #ifdef dbg
                std::cout << "Received a DXGI_ERROR_WAIT_TIMEOUT." << std::endl;
                #endif
                return false;
            }
            default:
            {
                #ifdef dbg
                std::cout << "Error: failed to acquire frame.\n" << std::endl;
                #endif
                return false;
            }
         };

        ID3D11Texture2D* staging_tex;
        hr = d3d_device->CreateTexture2D(&tex_desc, nullptr, &staging_tex);

        if (hr != S_OK)
        {
            #ifdef dbg
            std::printf("Failed to create the 2D texture, error: %ld.\n", hr);
            #endif
            return false;
        }

        d3d_context->CopyResource(staging_tex, tex);
        duplication->ReleaseFrame();

        D3D11_MAPPED_SUBRESOURCE map;

        while (d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map) == DXGI_ERROR_WAS_STILL_DRAWING) Sleep(UPDATE_TIME_MS);

        d3d_context->Unmap(staging_tex, 0);
        staging_tex->Release();
        d3d_context->Release();

    #ifdef dbg
            //std::cout << "rowPitch = " << map.RowPitch << " depthPitch = " << map.DepthPitch << " bufLen = " << bufLen << std::endl;
    #endif

        buf = reinterpret_cast<unsigned char*>(map.pData);
        //memcpy(buf, map.pData, map.DepthPitch);

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

                std::cout << "Retrying... (5 sec)" << std::endl;
            }
            #endif

            Sleep(5000);
        } while (hr != S_OK);

    #ifdef dbg
        std::cout << "Output duplication restarted successfully." << std::endl;
    #endif

        output1->Release();
    }

    void ReleaseDXResources()
    {
        duplication->ReleaseFrame();
        output1->Release();
        d3d_context->Release();
        d3d_device->Release();
    }
};

void getGDISnapshot(unsigned char* buf)
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
    Sleep(UPDATE_TIME_MS);

    SelectObject(memoryDC, oldObj);
    DeleteObject(hBitmap);
    DeleteObject(oldObj);
    DeleteDC(memoryDC);
}

void setGDIBrightness(WORD brightness, float gdiv, float bdiv)
{
    if (brightness > DEFAULT_BRIGHTNESS) {
        return;
    }

    WORD gammaArr[3][256];

    for (WORD i = 0; i < 256; ++i)
    {
        WORD gammaVal = i * brightness;

        gammaArr[0][i] = WORD (gammaVal);
        gammaArr[1][i] = WORD (gammaVal / gdiv);
        gammaArr[2][i] = WORD (gammaVal / bdiv);
    }

    SetDeviceGammaRamp(screenDC, gammaArr);
}

//Arguments to be passed to the AdjustBrightness thread
struct Args {
    short imgDelta = 0;
    size_t threadCount = 0;
    MainWindow* w;
    unsigned short imgBr;
};

void adjustBrightness(Args &args)
{
    size_t threadId = ++args.threadCount;

    #ifdef dbg
        std::cout << "Thread " << threadId << " started..." << std::endl;
    #endif

    short sleeptime = (100 - args.imgDelta / 4) / SPEED;
    args.imgDelta = 0;

    targetScrBr = DEFAULT_BRIGHTNESS - args.imgBr + OFFSET;

    if		(targetScrBr > MAX_BRIGHTNESS) targetScrBr = MAX_BRIGHTNESS;
    else if (targetScrBr < MIN_BRIGHTNESS) targetScrBr = MIN_BRIGHTNESS;

    if (scrBr < targetScrBr) sleeptime /= 3;

    while (scrBr != targetScrBr && threadId == args.threadCount)
    {
        if (scrBr < targetScrBr)
        {
            ++scrBr;
        }
        else --scrBr;

        setGDIBrightness(scrBr, gdivs[TEMP-1], bdivs[TEMP-1]);

        if(args.w->isVisible()) args.w->updateBrLabel();

        if (scrBr == MIN_BRIGHTNESS || scrBr == MAX_BRIGHTNESS)
        {
            targetScrBr = scrBr;
            break;
        }

        Sleep(sleeptime);
    }

    #ifdef dbg
    std::printf("Thread %zd finished. Stopped: %d\n", threadId, threadId < args.threadCount);
    #endif
}

void app(MainWindow* wnd, DXGIDupl &dx, bool useGDI)
{
    #ifdef dbg
    std::cout << "Starting routine..." << std::endl;
    #endif

    USHORT imgBr      = DEFAULT_BRIGHTNESS;
    USHORT oldImgBr   = DEFAULT_BRIGHTNESS;
    UCHAR  oldMin     = MIN_BRIGHTNESS;
    UCHAR  oldMax     = MAX_BRIGHTNESS;
    USHORT oldOffset  = OFFSET;

    //Buffer to store screen pixels
    unsigned char* buf = nullptr;
    bufLen = calcBufLen(w, h);
    if(useGDI) buf = new unsigned char[bufLen];

    Args args;
    args.w = wnd;

    bool forceChange = true;

    while (!wnd->quitClicked)
    {
        if (!useGDI)
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

        imgBr = getBrightness(buf);

        args.imgBr = imgBr;
        args.imgDelta += abs(oldImgBr - imgBr);

        if (args.imgDelta > THRESHOLD || forceChange)
        {
            std::thread t(adjustBrightness, std::ref(args));
            t.detach();

            forceChange = false;
        }

        if (MIN_BRIGHTNESS != oldMin || MAX_BRIGHTNESS != oldMax || OFFSET != oldOffset) {
            forceChange = true;
        }

        oldImgBr = imgBr;
        oldMin = MIN_BRIGHTNESS;
        oldMax = MAX_BRIGHTNESS;
        oldOffset = OFFSET;
    }

    setGDIBrightness(DEFAULT_BRIGHTNESS, 1, 1);
    if(useGDI) delete[] buf;
    else dx.ReleaseDXResources();
    QApplication::quit();
}

void checkInstance();
void checkGammaRange();
void readSettings();

int main(int argc, char *argv[])
{
    #ifdef dbg
    FILE *f1, *f2, *f3;
    AllocConsole();
    freopen_s(&f1, "CONIN$", "r", stdin);
    freopen_s(&f2, "CONOUT$", "w", stdout);
    freopen_s(&f3, "CONOUT$", "w", stderr);
    #endif
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    checkInstance();

    DXGIDupl dx;
    bool useGDI = false;

    if (!dx.initDXGI() || useGDI)
    {
        useGDI = true;
        UPDATE_TIME_MIN = 1000;
        UPDATE_TIME_MAX = 5000;
    }
    else
    {
        UPDATE_TIME_MIN = 10;
        UPDATE_TIME_MAX = 500;
    }

    readSettings();
    checkGammaRange();

    QApplication a(argc, argv);
    MainWindow wnd;

    std::thread t(app, &wnd, std::ref(dx), useGDI);
    t.detach();

    return a.exec();
}

//////////////////////////////////////////////

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
        std::cout << "Gamma registry key found." << std::endl;
#endif
        return;
    }

#ifdef dbg
        std::cout << "Gamma registry key not found. Proceeding to add..." << std::endl;
#endif

    HKEY hKey;

    s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

    if (s == ERROR_SUCCESS)
    {
#ifdef dbg
        std::cout << "Gamma registry key created." << std::endl;
#endif
        DWORD val = 256;

        s = RegSetValueExW(hKey, L"GdiICMGammaRange", 0, REG_DWORD, LPBYTE(&val), sizeof(val));

        if (s == ERROR_SUCCESS)
        {
            MessageBoxW(nullptr, L"Gammy has extended the brightness range. Restart to apply the changes.", L"Gammy", 0);
#ifdef dbg
            std::cout << "Gamma registry value set." << std::endl;
#endif
        }
#ifdef dbg
        else std::cout << "Error when setting Gamma registry value." << std::endl;
#endif
    }
#ifdef dbg
    else
    {
        std::cout << "Error when creating/opening gamma RegKey." << std::endl;

        if (s == ERROR_ACCESS_DENIED)
        {
            std::cout << "Access denied." << std::endl;
        }
    }
#endif

    if (hKey) RegCloseKey(hKey);
}

void readSettings()
{
    std::string filename = "gammySettings.cfg";

#ifdef dbg
    std::cout << "Opening settings file..." << std::endl;
#endif
    std::fstream file(filename, std::fstream::in | std::fstream::out | std::fstream::app);

    if(file.is_open())
    {
        #ifdef dbg
        std::cout << "File opened successfully." << std::endl;
        #endif
        if(std::filesystem::file_size(filename) == 0)
        {
            #ifdef dbg
            std::cout << "File is empty. Filling with defaults..." << std::endl;
            #endif

            std::string newLines [] =
            {
                "minBrightness=" + std::to_string(MIN_BRIGHTNESS),
                "maxBrightness=" + std::to_string(MAX_BRIGHTNESS),
                "offset=" + std::to_string(OFFSET),
                "speed=" + std::to_string(SPEED),
                "temp=" + std::to_string(TEMP),
                "threshold=" + std::to_string(THRESHOLD),
                "updateRate=" + std::to_string(UPDATE_TIME_MS)
            };

            for(auto s : newLines) file << s << std::endl;

            file.close();
            return;
        }

        std::string line;
        int values[settingsCount];
        int c = 0;

        #ifdef dbg
        std::cout << "Parsing settings file..." << std::endl;
        #endif

        while (getline(file, line))
        {
            #ifdef dbg
            std::cout << "Parsing line: " << line << std::endl;
            #endif

            if(line != "") values[c++] = std::stoi(line.substr(line.find("=") + 1));
        }

        MIN_BRIGHTNESS = UCHAR(values[0]);
        MAX_BRIGHTNESS = UCHAR(values[1]);
        OFFSET         = USHORT(values[2]);
        SPEED          = UCHAR(values[3]);
        TEMP           = UCHAR(values[4]);
        THRESHOLD      = UCHAR(values[5]);
        UPDATE_TIME_MS = USHORT(values[6]);

        if(UPDATE_TIME_MS < UPDATE_TIME_MIN) UPDATE_TIME_MS = UPDATE_TIME_MIN;
        if(UPDATE_TIME_MS > UPDATE_TIME_MAX) UPDATE_TIME_MS = UPDATE_TIME_MAX;

        file.close();
    }
}

int calcBufLen(int &w, int &h)
{
    int x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    w = x2 - x1;
    h = y2 - y1;
    return w * 4 * h;
}
