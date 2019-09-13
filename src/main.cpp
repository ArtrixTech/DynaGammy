/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QApplication>

#include "mainwindow.h"
#include "main.h"
#include "utils.h"

#ifdef _WIN32
    #include "dxgidupl.h"

    #pragma comment(lib, "gdi32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "DXGI.lib")
    #pragma comment(lib, "D3D11.lib")
    #pragma comment(lib, "Advapi32.lib")
#else
    #include "x11.h"
    #include <unistd.h>
    #include <signal.h>
#endif

#include <array>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

int scr_br = default_brightness; //Current screen brightness

int	polling_rate_min = 10;
int	polling_rate_max = 500;

#ifdef _WIN32
const HDC screenDC = GetDC(nullptr);
const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
const uint64_t screen_res = w * h;
const uint64_t len = screen_res * 4;
#else
static bool* sig_received {}; //Points to wnd.quit
#endif

struct Args
{
    int img_br = 0;
    int target_br = 0;
    int img_delta = 0;
    size_t callcnt = 0;

    std::mutex mtx;
    std::condition_variable cvr;
    MainWindow* w = nullptr;

#ifndef _WIN32
    X11* x11 = nullptr;
#endif
};

void adjustBrightness(Args &args)
{
    size_t c = 0;
    size_t old_c = 0;

    while(!args.w->quit)
    {
        {
#ifdef dbgthr
            std::cout << "adjustBrightness: waiting (" << c << ")\n";
#endif
            std::unique_lock<std::mutex> lock(args.mtx);
            args.cvr.wait(lock, [&]{return args.callcnt > old_c;});
        }

        c = args.callcnt;

#ifdef dbgthr
        std::cout << "adjustBrightness: working (" << c << ")\n";
#endif

        int sleeptime = (100 - args.img_delta / 4) / cfg[Speed];
        args.img_delta = 0;

        if (scr_br < args.target_br) sleeptime /= 3;

        while (c == args.callcnt && args.w->run)
        {
            if     (scr_br < args.target_br) ++scr_br;
            else if(scr_br > args.target_br) --scr_br;
            else break;

            if(!args.w->quit)
            {
                #ifdef _WIN32
                setGDIBrightness(scr_br, cfg[Temp]);
                #else
                args.x11->setXF86Brightness(scr_br, cfg[Temp]);
                #endif
            }

            if(args.w->isVisible()) args.w->updateBrLabel();

            std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
        }

        old_c = c;

#ifdef dbgthr
        std::cout << "adjustBrightness: complete (" << c << ")\n";
#endif
    }
}

void app(Args &args)
{
    #ifdef dbg
    std::cout << "Starting screenshots\n";
    #endif

    int old_imgBr  = default_brightness;
    int old_min    = default_brightness;
    int old_max    = default_brightness;
    int old_offset = default_brightness;

    bool force = false;
    args.w->force = &force;

    #ifdef _WIN32
    DXGIDupl dx;
    bool useDXGI = dx.initDXGI();

    if (!useDXGI)
    {
        polling_rate_min = 1000;
        polling_rate_max = 5000;
        args.w->updatePollingSlider(polling_rate_min, polling_rate_max);
    }
    #else
    const uint64_t screen_res = args.x11->getWidth() * args.x11->getHeight();
    const uint64_t len = screen_res * 4;

    args.x11->setXF86Brightness(scr_br, cfg[Temp]);
    #endif

    //Buffer to store screen pixels
    std::vector<uint8_t> buf(len);

    std::once_flag f;
    std::mutex m;

    std::thread t1(adjustBrightness, std::ref(args));

    while (!args.w->quit)
    {
        {
            std::unique_lock<std::mutex> lock(m);

            args.w->pausethr->wait(lock, [&]
            {
                 return args.w->run;
            });
        }

#ifdef _WIN32
        if (useDXGI)
        {
            while (!dx.getDXGISnapshot(buf.data()))
            {
                dx.restartDXGI();
            }
        }
        else
        {
            getGDISnapshot(buf.data(), len);
            Sleep(cfg[Polling_rate]));
        }
#else
        args.x11->getX11Snapshot(buf.data());
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg[Polling_rate]));
#endif
        args.img_br = calcBrightness(buf.data(), screen_res);
        args.img_delta += abs(old_imgBr - args.img_br);

        std::call_once(f, [&](){ args.img_delta = 0; });

        if (args.img_delta > cfg[Threshold] || force)
        {
            args.target_br = default_brightness - args.img_br + cfg[Offset];

            if (args.target_br > cfg[MaxBr])
            {
                 args.target_br = cfg[MaxBr];
            }
            else if (args.target_br < cfg[MinBr])
            {
                 args.target_br = cfg[MinBr];
            }

#ifdef dbgbr
            std::cout << scr_br << " -> " << args.target_br << " | " << args.img_delta << '\n';
#endif

            if(args.target_br != scr_br)
            {
                ++args.callcnt;
    #ifdef dbgthr
                std::cout << "app: ready (" << args.callcnt << ")\n";
    #endif
                args.cvr.notify_one();
            }
            else args.img_delta = 0;

            force = false;
        }

        if (cfg[MinBr] != old_min || cfg[MaxBr] != old_max || cfg[Offset] != old_offset)
        {
            force = true;
        }

        old_imgBr  = args.img_br;
        old_min    = cfg[MinBr];
        old_max    = cfg[MaxBr];
        old_offset = cfg[Offset];
    }

#ifdef _WIN32
    setGDIBrightness(default_brightness, 1);
#else
    args.x11->setInitialGamma(args.w->set_previous_gamma);
#endif

    ++args.callcnt;
    args.cvr.notify_one();

#ifdef dbgthr
    std::cout << "app: notified children to quit (" << args.callcnt << ")\n";
#endif

    t1.join();
    QApplication::quit();
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    checkInstance();

    #ifdef dbg
    FILE *f1, *f2, *f3;
    AllocConsole();
    freopen_s(&f1, "CONIN$", "r", stdin);
    freopen_s(&f2, "CONOUT$", "w", stdout);
    freopen_s(&f3, "CONOUT$", "w", stderr);
    #endif

    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

    checkGammaRange();
#else
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);

    X11 x11;
#endif

    QApplication a(argc, argv);

    std::condition_variable pausethr;

#ifdef _WIN32
    MainWindow wnd(nullptr, &pausethr);
#else
    MainWindow wnd(&x11, &pausethr);
    sig_received = &wnd.quit;
#endif

    Args args;
    args.w = &wnd;

#ifndef _WIN32
    args.x11 = &x11;
#endif

    std::thread t1(app, std::ref(args));

    a.exec();
    t1.join();

    QApplication::quit();
}

#ifndef _WIN32
void sig_handler(int signo)
{
    switch(signo)
    {
        case SIGINT:
        {
            #ifdef dbg
            std::cout << "Received SIGINT.\n";
            #endif
            break;
        }
        case SIGTERM:
        {
            #ifdef dbg
            std::cout << "Received SIGTERM.\n";
            #endif
            break;
        }
        case SIGQUIT:
        {
            #ifdef dbg
            std::cout << "Received SIGQUIT.\n";
            #endif
            break;
        }
    }

    updateConfig();

    if(sig_received)
    {
        *sig_received = true;
    }
    else
    {
        _exit(0);
    }
}
#endif
