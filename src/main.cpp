/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QApplication>
#include <QTime>

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
#include <iomanip>
#include <chrono>
#include <ctime>
#include <charconv>

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
    convar  temp_cv;
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
        LOGD << "Waiting (" << c << ')';

        args.adjustbr_cv.wait(lock, [&]{ return args.callcnt > prev_c; });

        c = args.callcnt;

        LOGD << "Working (" << c << ')';

        int speed = cfg["speed"];

        int sleeptime = (100 - args.img_delta / 4) / speed;
        args.img_delta = 0;

        if (scr_br < args.target_br) sleeptime /= 3;

        while (c == args.callcnt && args.w->run_ss_thread)
        {
            if     (scr_br < args.target_br) ++scr_br;
            else if(scr_br > args.target_br) --scr_br;
            else break;

            if(!args.w->quit)
            {
                if constexpr (os == OS::Windows)
                {
                    setGDIGamma(scr_br, cfg["temp_step"]);
                }
#ifndef _WIN32 // @TODO: replace this, as it defeats the whole purpose of the constexpr check
                else {args.x11->setXF86Gamma(scr_br, cfg["temp_step"]); }
#endif
            }
            else break;

            if(args.w->isVisible()) args.w->updateBrLabel();

            std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
        }

        prev_c = c;
    }

       LOGD << "Complete (" << c << ')';
}

void adjustTemperature(Args &args)
{
    using namespace std::this_thread;
    using namespace std::chrono;
    using namespace std::chrono_literals;

    std::string t_start;
    std::string t_end;

    bool needs_change = true;
    args.w->temp_needs_change = &needs_change;

    std::mutex m;
    std::unique_lock lock(m);

    bool is_past_decrease = false;
    bool is_past_increase = false;

    bool restore          = false;
    bool decreased_state  = false;

    while (!args.w->quit)
    {
        sleep_for(5s);

        args.w->temp_cv->wait(lock, [&]{ return args.w->run_temp_thread; });

        std::string time_start_str  = cfg["time_start"];
        std::string start_hour      = time_start_str.substr(0, 2);
        std::string start_min       = time_start_str.substr(3, 2);

        std::string time_end_str    = cfg["time_end"];
        std::string end_hour        = time_end_str.substr(0, 2);
        std::string end_min         = time_end_str.substr(3, 2);

        QTime start_time            = QTime(std::stoi(start_hour), std::stoi(start_min));
        QTime end_time              = QTime(std::stoi(end_hour), std::stoi(end_min));

        QDateTime start_datetime    = QDateTime::currentDateTime();
        QDateTime end_datetime      = start_datetime.addDays(1);

        start_datetime.setTime(start_time);
        end_datetime.setTime(end_time);

        LOGI << "Temp. decreasing at: " << start_datetime.toString();
        LOGI << "Temp. increasing at: " << end_datetime.toString();

        is_past_decrease = QDateTime::currentDateTime() > start_datetime;
        is_past_increase = QDateTime::currentDateTime() > end_datetime;

        if(is_past_decrease)
        {
            LOGI << "We are past decrease time.";
        }
        else if(is_past_increase)
        {
            LOGI << "We are past increase time.";
        }
        else
        {
            LOGI << "No change needed";
            continue;
        }

        if(restore && decreased_state)
        {
            LOGI << "Temperature has been restored.";
            continue;
        }

        if(decreased_state)
        {
            LOGI << "Temperature is in decreased state already.";

            if(needs_change)
            {
                LOGI << "However, a change has been detected.";
            }
            else if(is_past_increase && !restore)
            {
                LOGI << "However, it's time to restore it.";
                restore = true;
                needs_change = true;
            }
            else continue;
        }

        LOGI << "Adjusting temperature...";

        int target_temp;

        if(restore) target_temp = cfg["start_temp"];
        else        target_temp = cfg["target_temp"];

        // Map kelvin temperature to a step in the temperature array
        int target_step = ((max_temp_kelvin - target_temp) * temp_steps) / (max_temp_kelvin - min_temp_kelvin) + temp_mult;

        while (needs_change && args.w->run_temp_thread)
        {
            int temp_step = cfg["temp_step"];

            if     (temp_step < target_step) ++temp_step;
            else if(temp_step > target_step) --temp_step;
            else
            {
                decreased_state = is_past_decrease;
                needs_change = false;
                break;
            }

            if(!args.w->quit)
            {
                if constexpr (os == OS::Windows)
                {
                    setGDIGamma(scr_br, temp_step);
                }
#ifndef _WIN32
                else { args.x11->setXF86Gamma(scr_br, temp_step); }
#endif
            }
            else break;

            args.w->setTempSlider(temp_step);

            cfg["temp_step"] = temp_step;

            sleep_for(50ms);
        }
    }
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

    args.x11->setXF86Gamma(scr_br, cfg["temp_step"]);
#endif

    LOGD << "Buffer size: " << len;

    // Buffer to store screen pixels
    std::vector<uint8_t> buf(len);

    std::thread t1(adjustBrightness, std::ref(args));
    std::thread t2(adjustTemperature, std::ref(args));

    std::once_flag f;
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);

    while (!args.w->quit)
    {
        args.w->auto_cv->wait(lock, [&] { return args.w->run_ss_thread; });

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

        std::this_thread::sleep_for(std::chrono::milliseconds(cfg["polling_rate"]));
#endif

        args.img_br     = calcBrightness(buf);
        args.img_delta += abs(prev_imgBr - args.img_br);

        std::call_once(f, [&](){ args.img_delta = 0; });

        if (args.img_delta > cfg["threshold"] || force)
        {
            int offset = cfg["offset"];

            args.target_br = default_brightness - args.img_br + offset;

            if (args.target_br > cfg["max_br"]) {
                args.target_br = cfg["max_br"];
            }
            else
            if (args.target_br < cfg["min_br"]) {
                args.target_br = cfg["min_br"];
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

        if (cfg["min_br"] != prev_min || cfg["max_br"] != prev_max || cfg["offset"] != prev_offset)
        {
            force = true;
        }

        prev_imgBr  = args.img_br;
        prev_min    = cfg["min_br"];
        prev_max    = cfg["max_br"];
        prev_offset = cfg["offset"];
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

    LOGD << "Notified children to quit (" << args.callcnt << ')';

    _exit(0);

    t1.join();
    t2.join();
}

int main(int argc, char *argv[])
{
    static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 5, 1);
    static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

    plog::init(plog::Severity(plog::info), &console_appender);

    read();

    plog::get()->addAppender(&file_appender);
    plog::get()->setMaxSeverity(cfg["log_lvl"]);

#ifdef _WIN32
    checkInstance();

    if(cfg["log_lvl"] > plog::none)
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
    convar temp_cv;

#ifdef _WIN32
    MainWindow wnd(nullptr, &auto_cv, $temp_cv);
#else
    MainWindow wnd(&x11, &auto_cv, &temp_cv);

    cv_ptr          = &auto_cv;
    quit_ptr        = &wnd.quit;
    run_ptr         = &wnd.run_ss_thread;
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

    save();

    if(run_ptr && quit_ptr && cv_ptr)
    {
        *run_ptr = true;
        *quit_ptr = true;
        cv_ptr->notify_one();
    }
    else _exit(0);
}
#endif
