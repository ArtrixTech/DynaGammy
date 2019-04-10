/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "mainwindow.h"
#include "main.h"
#include <QApplication>
#include <Windows.h>
#include <vector>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
//#include <thread>
//#include <dxgitype.h>
//#include <ntddvdeo.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Advapi32.lib")

HDC screenDC = GetDC(nullptr); //GDI Device Context of entire screen

int x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
int yy1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
int x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
int y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);
int w = x2 - x1;
int h = y2 - yy1;
const unsigned int len = w * 4 * h;

unsigned char	MIN_BRIGHTNESS = 176;
unsigned char	MAX_BRIGHTNESS = DEFAULT_BRIGHTNESS;
unsigned short	OFFSET = 70;
unsigned char	SPEED = 2;
unsigned char	TEMP = 1;
unsigned char	THRESHOLD = 32;
unsigned short	UPDATE_TIME_MS = 100;
unsigned short	UPDATE_TIME_MIN = 10;
unsigned short	UPDATE_TIME_MAX = 500;

WORD brightness = DEFAULT_BRIGHTNESS; //Current image brightness
WORD oldBrightness = DEFAULT_BRIGHTNESS;

unsigned short res = DEFAULT_BRIGHTNESS;		//Current screen brightness
unsigned short targetRes = DEFAULT_BRIGHTNESS;  //Difference between max and current image brightness

short delta = 0; //Difference between old and current image brightness

short sleeptime;

UCHAR oldMin = MIN_BRIGHTNESS;
UCHAR oldMax = MAX_BRIGHTNESS;
USHORT oldOffset = OFFSET;

bool useGDI = false;

size_t stop = 0;

bool CheckOneInstance(const char* name)
{
    HANDLE hStartEvent = CreateEventA(nullptr, TRUE, FALSE, name);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hStartEvent);
        hStartEvent = nullptr;
        ///MessageBoxW(0, L"Gammy is already running.", L"Gammy", 0);
        exit(0);
    }

    return true;
}

void readSettings()
{
#ifdef _db
    printf("Reading settings...\n");
#endif

    std::string filename = "gammySettings.cfg";

    std::fstream file;
    file.open(filename);

    std::cout << "File open: " << file.is_open() << std::endl;

    if(file.is_open())
    {
        std::string line;
        int lines[settingsCount];
        USHORT c = 0;

        while (getline(file, line))
        {
            line = line.substr(line.find("=") + 1);

            lines[c++] = std::stoi(line);
        }

        MIN_BRIGHTNESS = lines[0];
        MAX_BRIGHTNESS = lines[1];
        OFFSET         = lines[2];
        SPEED          = lines[3];
        TEMP           = lines[4];
        THRESHOLD      = lines[5];
        UPDATE_TIME_MS = lines[6];

        file.close();
    }

    if (useGDI)
    {
        if(UPDATE_TIME_MS < UPDATE_TIME_MIN) UPDATE_TIME_MS = UPDATE_TIME_MIN;
    }
}

void getBrightness(LPBYTE const &buf)
{
    UCHAR r = buf[2];
    UCHAR g = buf[1];
    UCHAR b = buf[0];

    UINT colorSum = 0;

    for (UINT i = len; i > 0; i -= 4)
    {
        r = buf[i + 2];
        g = buf[i + 1];
        b = buf[i];

        colorSum += (r + g + b) / 3;
    }

    brightness = colorSum / (w * h); //Assigns a value between 0 and 255
}

void setGDIBrightness(WORD brightness, float gdiv, float bdiv)
{
    if (brightness > DEFAULT_BRIGHTNESS) {
        return;
    }

    WORD gammaArr[3][256];

    for (USHORT i = 0; i < 256; ++i)
    {
        WORD gammaVal = (WORD)(i * brightness);

        gammaArr[0][i] = (WORD)(gammaVal);
        gammaArr[1][i] = (WORD)(gammaVal / gdiv);
        gammaArr[2][i] = (WORD)(gammaVal / bdiv);
    }

    SetDeviceGammaRamp(screenDC, gammaArr);
}

ID3D11Device*			d3d_device;
ID3D11DeviceContext*	d3d_context;
IDXGIOutput1*			output1;
IDXGIOutputDuplication* duplication;
D3D11_TEXTURE2D_DESC	tex_desc;

//Buffer to store screen pixels
LPBYTE buf;

HWND hWnd;

bool initDXGI() {
    #ifdef _db
    printf("Initializing DXGI...\n");
    #endif

    HRESULT hr;
    IDXGIFactory1* pFactory;

    //Retrieve a IDXGIFactory to enumerate the adapters
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));

    if (hr != S_OK)
    {
        #ifdef _db
        printf("Error: failed to retrieve the IDXGIFactory.\n");
        getchar();
        #endif
        return false;
    }

    IDXGIAdapter1* pAdapter;
    std::vector<IDXGIAdapter1*> vAdapters; //GPUs vector
    std::vector<IDXGIOutput*>   vOutputs; //Monitors vector

    UINT i = 0;

    while (pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++i;
    }

    pFactory->Release();

#ifdef _db
    //Get GPU info
    for (UINT i = 0; i < vAdapters.size(); ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter = vAdapters[i];
        hr = pAdapter->GetDesc1(&desc);

        if (hr != S_OK)
        {
            printf("Error: failed to get a description for the adapter: %lu\n", i);
            continue;
        }

        wprintf(L"Adapter %i: %lS\n", i, desc.Description);
    }
#endif

    //Get the monitors attached to the GPUs. We don't use IDXGIOutput1 because it lacks EnumOuputs.
    IDXGIOutput* output;

    UINT dx;

    for (UINT i = 0; i < vAdapters.size(); ++i)
    {
        dx = 0;
        pAdapter = vAdapters[i];
        while (pAdapter->EnumOutputs(dx, &output) != DXGI_ERROR_NOT_FOUND)
        {
            #ifdef _db
            printf("Found monitor %d on adapter %lu\n", dx, i);
            #endif
            vOutputs.push_back(output);
            ++dx;
        }
    }

    if (vOutputs.size() <= 0)
    {
        #ifdef _db
        printf("Error: no outputs found (%zu).\n", vOutputs.size());
        getchar();
        #endif

        return false;
    }

#ifdef _db
    //Print monitor info.
    for (size_t i = 0; i < vOutputs.size(); ++i)
    {
        DXGI_OUTPUT_DESC desc;
        output = vOutputs[i];
        hr = output->GetDesc(&desc);

        if (hr != S_OK) {
            printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %lu.\n", i);
            continue;
        }

        wprintf(L"Monitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'y' : 'n');
    }
#endif

    //Create a Direct3D device to access the OutputDuplication interface
    IDXGIAdapter1* d3d_adapter;
    D3D_FEATURE_LEVEL d3d_feature_level;

    UINT use_adapter = 0;

    if (vAdapters.size() <= use_adapter)
    {
        #ifdef _db
        printf("Invalid adapter index: %d, we only have: %zu - 1\n", use_adapter, vAdapters.size());
        getchar();
        #endif

        return false;
    }

    d3d_adapter = vAdapters[use_adapter];

    if (!d3d_adapter)
    {
        #ifdef _db
        printf("Error: the stored adapter is NULL.\n");
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
        #ifdef _db
        printf("Error: failed to create the D3D11 Device.\n");

        if (hr == E_INVALIDARG)
        {
            printf("Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?.\n");
        }

        getchar();
        #endif

        return false;
    }

    UINT use_monitor = 0;
    output = vOutputs[use_monitor];

    if (vOutputs.size() <= use_monitor)
    {
        #ifdef _db
        printf("Invalid monitor index: %d, we only have: %zu - 1\n", use_monitor, vOutputs.size());
        getchar();
        #endif

        return false;
    }

    if (!output)
    {
        #ifdef _db
        printf("No valid output found. The output is NULL.\n");
        getchar();
        #endif

        return false;
    }

    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);

    if (hr != S_OK)
    {
        #ifdef _db
        printf("Error: failed to query the IDXGIOutput1 interface.\n");
        getchar();
        #endif

        return false;
    }

    DXGI_OUTPUT_DESC output_desc;
    hr = output->GetDesc(&output_desc);

    if (hr != S_OK)
    {
        #ifdef _db
        printf("Error: failed to get output description.\n");
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
        #ifdef _db
        printf("Error: DuplicateOutput failed.\n");
        getchar();
        #endif

        return false;
    }

    //Get output duplication description.
    //DXGI_OUTDUPL_DESC duplication_desc;
    //duplication->GetDesc(&duplication_desc);
    //If duplication_desc.DesktopImageInSystemMemory is true you can use MapDesktopSurface/UnMapDesktopSurface to retrieve the pixels.
    //Otherwise you need to use a surface.

    output1->Release();
    d3d_device->Release();

    for (UINT i = 0; i < vAdapters.size(); ++i) vAdapters[i]->Release();
    for (UINT i = 0; i < vOutputs.size(); ++i) vOutputs[i]->Release();

    return true;
}

bool getDXGISnapshot(LPBYTE &buf) {
    HRESULT hr;

    ID3D11Texture2D* tex;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    IDXGIResource* desktop_resource;

    if (!duplication) {
        #ifdef _db
        printf("Duplication is a null pointer.\n");
        #endif
        return false;
    }

    hr = duplication->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);

    if (hr == S_OK)
    {
        hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex); //Get the texture interface

        desktop_resource->Release();
        tex->Release();

        if (hr != S_OK) {
            #ifdef _db
            printf("Error: failed to query the ID3D11Texture2D interface on the IDXGIResource.\n");
            #endif
            return false;
        }
    }
    else if (hr == DXGI_ERROR_ACCESS_LOST) {
        #ifdef _db
        printf("Received a DXGI_ERROR_ACCESS_LOST.\n");
        #endif
        return false;
    }
    else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        #ifdef _db
        printf("Received a DXGI_ERROR_WAIT_TIMEOUT.\n");
        #endif
        return false;
    }
    else {
        #ifdef _db
        printf("Error: failed to acquire frame.\n");
        #endif
        return false;
    }

    ID3D11Texture2D* staging_tex;
    hr = d3d_device->CreateTexture2D(&tex_desc, NULL, &staging_tex);

    #ifdef _db
    if (hr == E_INVALIDARG) {
        #ifdef _db
        printf("Error: received E_INVALIDARG when trying to create the texture.\n");
        #endif

        return false;
    }
    else if (hr != S_OK) {
        #ifdef _db
        printf("Failed to create the 2D texture, error: %d.\n", hr);
        #endif
        return false;
    }
    #endif

    //Map the desktop surface (not supported on all PCs, mapping staging texture instead)
    /*DXGI_MAPPED_RECT mapped_rect;
    hr = duplication->MapDesktopSurface(&mapped_rect);
    if (hr == S_OK) {
        hr = duplication->UnMapDesktopSurface();
    }
    else if (hr == DXGI_ERROR_UNSUPPORTED) {
        printf("MapDesktopSurface unsupported. Map staging texture instead\n");
    }*/

    d3d_context->CopyResource(staging_tex, tex);

    duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE map;

    //UINT c = 0;

    do
    {
        Sleep(UPDATE_TIME_MS);

        hr = d3d_context->Map(staging_tex, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &map);

    } while (hr == DXGI_ERROR_WAS_STILL_DRAWING);

    d3d_context->Unmap(staging_tex, 0);

    staging_tex->Release();
    d3d_context->Release();

    if (map.DepthPitch > 0)
    {
        //We could access map.pData directly to save some RAM, but it causes a crash on some systems.
        memcpy(buf, map.pData, map.DepthPitch);

        getBrightness(buf);
    }

    return true;
}

void reInit() {

    HRESULT hr;

    do {
        hr = output1->DuplicateOutput(d3d_device, &duplication);

        #ifdef _db
        if (hr != S_OK) {

            printf("Unable to duplicate output. Reason: ");

            switch (hr) {
            case E_INVALIDARG:	 printf("E_INVALIDARG\n"); break;
            case E_ACCESSDENIED: printf("E_ACCESSDENIED\n"); break;
            case DXGI_ERROR_UNSUPPORTED: printf("E_DXGI_ERROR_UNSUPPORTED\n"); break;
            case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: printf("DXGI_ERROR_NOT_CURRENTLY_AVAILABLE\n"); break;
            case DXGI_ERROR_SESSION_DISCONNECTED: printf("DXGI_ERROR_SESSION_DISCONNECTED\n"); break;
            }
        }
        #endif

        Sleep(5000);
    } while (hr != S_OK);

    output1->Release();
}

void ReleaseDXResources() {
    duplication->ReleaseFrame();
    output1->Release();
    d3d_context->Release();
    d3d_device->Release();
}

void getGDISnapshot(LPBYTE &buf)
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
    bminfoheader.biSizeImage = len;
    bminfoheader.biClrUsed = 0;
    bminfoheader.biClrImportant = 0;

    /*We can create the buffer here and delete[] it afterwards to make it look like we use less RAM, but that's shitty */
    //unsigned char* buf = new unsigned char[len];

    GetDIBits(memoryDC, hBitmap, 0, h, buf, (BITMAPINFO*)&bminfoheader, DIB_RGB_COLORS);

    getBrightness(buf);

    //delete[] buf;

    SelectObject(memoryDC, oldObj);
    DeleteObject(hBitmap);
    DeleteObject(oldObj);
    DeleteDC(memoryDC);
}

struct params {
    size_t t;
} lp;

void updateLabel() {
    //updateLabels(res, targetRes, lp.t, stop, sleeptime);
}

void adjustBrightness()
{
    size_t t = stop;

    #ifdef _db
    printf("Thread %zd started...\n", t);
    #endif

    sleeptime = (100 - delta / 4) / SPEED;
    delta = 0;

    targetRes = DEFAULT_BRIGHTNESS - brightness + OFFSET;

    if		(targetRes > MAX_BRIGHTNESS) targetRes = MAX_BRIGHTNESS;
    else if (targetRes < MIN_BRIGHTNESS) targetRes = MIN_BRIGHTNESS;

    if (res < targetRes) sleeptime /= 3;

    lp.t = t;
    //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)updateLabel, 0, 0, 0);

    while (res != targetRes && stop == t)
    {
        if (res < targetRes) ++res;
        else				 --res;

        setGDIBrightness(res, gdivs[TEMP-1], bdivs[TEMP-1]);
        //setWndBrightness(res);

        if (res == MIN_BRIGHTNESS || res == MAX_BRIGHTNESS)
        {
            targetRes = res;
            break;
        }

        Sleep(sleeptime);
    }

    #ifdef _db
    if (stop > t)
    {
        printf("Thread %zd stopped!\n", t);
        return;
    }

    printf("Thread %zd finished.\n", t);
    #endif
}

[[noreturn]] void app()
{
    #ifdef _db
    printf("Starting routine...\n");
    #endif

    bool forceChange = true;

    buf = new BYTE[len];

    while (true)
    {
        if (!useGDI)
        {
            while (!getDXGISnapshot(buf))
            {
                #ifdef _db
                printf("Screenshot failed. Retrying... (5 sec.)\n");
                #endif

                Sleep(5000);
                reInit();
            }
        }
        else
        {
            getGDISnapshot(buf);
            Sleep(UPDATE_TIME_MS);
        }

        delta += abs(oldBrightness - brightness);

        if (delta > THRESHOLD || forceChange)
        {
            ++stop;
            CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)adjustBrightness, nullptr, 0, nullptr);

            forceChange = false;
        }

        if (MIN_BRIGHTNESS != oldMin || MAX_BRIGHTNESS != oldMax || OFFSET != oldOffset) {
            forceChange = true;
        }

        oldBrightness = brightness;
        oldMin = MIN_BRIGHTNESS;
        oldMax = MAX_BRIGHTNESS;
        oldOffset = OFFSET;
    }
}

void checkGammaRange()
{
    LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM";
    LSTATUS s;

    s = RegGetValueW(HKEY_LOCAL_MACHINE, subKey, L"GdiICMGammaRange", RRF_RT_REG_DWORD, nullptr, nullptr, nullptr);

    if (s == ERROR_SUCCESS)
    {
#ifdef _db
        printf("Gamma registry key found.\n");
#endif
        return;
    }

#ifdef _db
        printf("Gamma registry key not found. Proceeding to add...\n");
#endif

    HKEY hKey;

    s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

    if (s == ERROR_SUCCESS)
    {
#ifdef _db
        printf("Gamma registry key created. \n");
#endif
        DWORD val = 256;

        s = RegSetValueExW(hKey, L"GdiICMGammaRange", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));

        if (s == ERROR_SUCCESS)
        {
            MessageBoxW(nullptr, L"Gammy has extended the brightness range. Restart to apply the changes.", L"Gammy", 0);
#ifdef _db
            printf("Gamma registry value set.\n");
#endif
        }
#ifdef _db
        else printf("Error when setting Gamma registry value.\n");
#endif
    }
#ifdef _db
    else
    {
        printf("Error when creating/opening gamma RegKey:");

        if (s == ERROR_ACCESS_DENIED)
        {
            printf(" Access denied.\n");
        }
    }
#endif

    if (hKey) RegCloseKey(hKey);
}

int main(int argc, char *argv[])
{
    readSettings();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    #ifdef _db
    FILE *f1, *f2, *f3;
    AllocConsole();
    freopen_s(&f1, "CONIN$", "r", stdin);
    freopen_s(&f2, "CONOUT$", "w", stdout);
    freopen_s(&f3, "CONOUT$", "w", stderr);
    #endif

    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

    CheckOneInstance("Gammy");

    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

    checkGammaRange();

    if (!initDXGI() || useGDI)
    {
        useGDI = true;
        UPDATE_TIME_MIN = 1000;
        UPDATE_TIME_MAX = 5000;
    }



    //std::thread start(app);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)app, nullptr, 0, nullptr);

    return a.exec();
}
