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

int scr_br = default_brightness; // Current screen brightness

int	polling_rate_min = 10;
int	polling_rate_max = 500;

#ifdef _WIN32
const uint64_t  w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN),
                h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN),
                screen_res = w * h,
                len = screen_res * 4;
#else

// To be used in unix signal handler
static bool *run_ptr, *quit_ptr;
static convar *cv_ptr;
#endif

struct Args
{
    convar  adjustbr_cv;
#ifndef _WIN32
    X11         *x11 {};
#endif
    MainWindow    *w {};
    int64_t callcnt = 0;
    int img_br      = 0;
    int target_br   = 0;
    int img_delta   = 0;
};

void adjustBrightness(Args &args)
{
    int64_t c = 0, prev_c = 0;

    std::mutex m;
    std::unique_lock<std::mutex> lock(m);

    while(!args.w->quit)
    {
#ifdef dbgthr
        std::cout << "adjustBrightness: waiting (" << c << ")\n";
#endif
        args.adjustbr_cv.wait(lock, [&]{ return args.callcnt > prev_c; });

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
                setGDIGamma(scr_br, cfg[Temp]);
#else
                args.x11->setXF86Gamma(scr_br, cfg[Temp]);
#endif
            }
            else break;

            if(args.w->isVisible()) args.w->updateBrLabel();

            std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
        }

        prev_c = c;

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

    int prev_imgBr  = 0,
        prev_min    = 0,
        prev_max    = 0,
        prev_offset = 0;

    bool force = false;
    args.w->force = &force;

#ifdef _WIN32
    DXGIDupl dx;
    bool useDXGI = dx.initDXGI();

    if (!useDXGI)
    {
        args.w->updatePollingSlider(polling_rate_min = 1000, polling_rate_max = 5000);
    }
#else
    const uint64_t screen_res = args.x11->getWidth() * args.x11->getHeight();
    const uint64_t len = screen_res * 4;

    args.x11->setXF86Gamma(scr_br, cfg[Temp]);
#endif

    // Buffer to store screen pixels
    std::vector<uint8_t> buf(len);

    std::thread t1(adjustBrightness, std::ref(args));

    std::once_flag f;
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);

    while (!args.w->quit)
    {
        args.w->auto_cv->wait(lock, [&]
        {
             return args.w->run;
        });

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
            getGDISnapshot(buf.data(), w, h);
            Sleep(cfg[Polling_rate]);
        }
#else
        args.x11->getX11Snapshot(buf);

        std::this_thread::sleep_for(std::chrono::milliseconds(cfg[Polling_rate]));
#endif

        args.img_br     = calcBrightness(buf);
        args.img_delta += abs(prev_imgBr - args.img_br);

        std::call_once(f, [&](){ args.img_delta = 0; });

        if (args.img_delta > cfg[Threshold] || force)
        {
            args.target_br = default_brightness - args.img_br + cfg[Offset];

            if (args.target_br > cfg[MaxBr]) {
                args.target_br = cfg[MaxBr];
            }
            else
            if (args.target_br < cfg[MinBr]) {
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
                args.adjustbr_cv.notify_one();
            }
            else args.img_delta = 0;

            force = false;
        }

        if (cfg[MinBr] != prev_min || cfg[MaxBr] != prev_max || cfg[Offset] != prev_offset)
        {
            force = true;
        }

        prev_imgBr  = args.img_br;
        prev_min    = cfg[MinBr];
        prev_max    = cfg[MaxBr];
        prev_offset = cfg[Offset];
    }

#ifdef _WIN32
    setGDIGamma(default_brightness, 1);
#else
    args.x11->setInitialGamma(args.w->set_previous_gamma);
#endif

    ++args.callcnt;
    args.adjustbr_cv.notify_one();

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

    Args thread_args;
    convar auto_cv;

#ifdef _WIN32
    MainWindow wnd(nullptr, &auto_cv);
#else
    MainWindow wnd(&x11, &auto_cv);

    cv_ptr = &auto_cv;
    quit_ptr = &wnd.quit;
    run_ptr = &wnd.run;
    thread_args.x11 = &x11;
#endif

    thread_args.w = &wnd;

    std::thread t1(app, std::ref(thread_args));

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

    saveConfig();

    if(run_ptr && quit_ptr && cv_ptr)
    {
        *run_ptr = true;
        *quit_ptr = true;
        cv_ptr->notify_one();
    }
    else _exit(0);
}
#endif
