/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QApplication>

#include "mainwindow.h"
#include "main.h"
#include "utils.h"
#include "cfg.h"

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

// Reflects the current screen brightness
int scr_br = default_brightness;

#ifndef _WIN32
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
        LOGD << "Waiting (" << c << ")";

        args.adjustbr_cv.wait(lock, [&]{ return args.callcnt > prev_c; });

        c = args.callcnt;

        LOGD << "Working (" << c << ")";

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
                if constexpr (os == OS::Windows)
                {
                    setGDIGamma(scr_br, cfg[Temp]);
                }
#ifndef _WIN32 // @TODO: replace this, as it defeats the whole purpose of the constexpr check
                else {args.x11->setXF86Gamma(scr_br, cfg[Temp]); }
#endif
            }
            else break;

            if(args.w->isVisible()) args.w->updateBrLabel();

            std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
        }

        prev_c = c;
    }

       LOGD << "Complete (" + std::to_string(c) + ")";
}

void recordScreen(Args &args)
{
    PLOGV << "recordScreen() start";

    int prev_imgBr  = 0,
        prev_min    = 0,
        prev_max    = 0,
        prev_offset = 0;

    bool force = false;
    args.w->force = &force;

#ifdef _WIN32
    const uint64_t  w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN),
                    h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN),
                    len = w * h * 4;

    LOGD << "Screen resolution: " << w  << "*" << h;

    DXGIDupl dx;

    bool useDXGI = dx.initDXGI();

    if (!useDXGI)
    {
        LOGE << "DXGI initialization failed. Using GDI instead";
        args.w->setPollingRange(1000, 5000);
    }

#else
    const uint64_t screen_res = args.x11->getWidth() * args.x11->getHeight();
    const uint64_t len = screen_res * 4;

    args.x11->setXF86Gamma(scr_br, cfg[Temp]);
#endif

    LOGD << "Screen bufsize: " << len;

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

        LOGV << "Taking screenshot";

#ifdef _WIN32
        if (useDXGI)
        {
            while (!dx.getDXGISnapshot(buf)) dx.restartDXGI();
        }
        else
        {
            getGDISnapshot(buf);
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg[Polling_rate]));
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

            LOGD << scr_br << " -> " << args.target_br << " delta: " << args.img_delta;

            if(args.target_br != scr_br)
            {
                ++args.callcnt;

                LOGD << "ready (" << args.callcnt << ')';

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

    if constexpr (os == OS::Windows)
    {
        setGDIGamma(default_brightness, 0);
    }
#ifndef _WIN32 // @TODO: replace this
    else args.x11->setInitialGamma(args.w->set_previous_gamma);
#endif

    ++args.callcnt;
    args.adjustbr_cv.notify_one();

    LOGV << std::string("Notified children to quit (" + std::to_string(args.callcnt) + ")");

    t1.join();
    QApplication::quit();
}

int main(int argc, char *argv[])
{
    static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 3, 3);
    static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

    readConfig();

    plog::init(plog::Severity(cfg[Debug]), &console_appender);

    if(cfg[Debug] >= plog::debug) plog::get()->addAppender(&file_appender);

#ifdef _WIN32
    checkInstance();

    if(cfg[Debug] >= plog::debug)
    {
        FILE *f1, *f2, *f3;
        AllocConsole();
        freopen_s(&f1, "CONIN$", "r", stdin);
        freopen_s(&f2, "CONOUT$", "w", stdout);
        freopen_s(&f3, "CONOUT$", "w", stderr);
    }

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

    cv_ptr          = &auto_cv;
    quit_ptr        = &wnd.quit;
    run_ptr         = &wnd.run;
    thread_args.x11 = &x11;
#endif

    thread_args.w = &wnd;

    std::thread t1(recordScreen, std::ref(thread_args));

    a.exec();
    t1.join();

    QApplication::quit();
}

#ifndef _WIN32
void sig_handler(int signo)
{
    LOGD_IF(signo == SIGINT) << "SIGINT received";
    LOGD_IF(signo == SIGTERM) << "SIGTERM received";
    LOGD_IF(signo == SIGQUIT) << "SIGQUIT received";

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
