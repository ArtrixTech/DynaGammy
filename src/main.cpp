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

            if(args.w->quit) break;

            if constexpr (os == OS::Windows)
            {
                setGDIGamma(scr_br, cfg["temp_step"]);
            }
#ifndef _WIN32
            else { args.x11->setXF86Gamma(scr_br, cfg["temp_step"]); }
#endif

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

    std::mutex m;
    std::unique_lock lock(m);

    int state = cfg["temp_state"];

    enum TempState {
        INITIAL,
        LOW_TEMP,
        HIGH_TEMP
    };

    bool start_date_reached   = false;
    bool end_date_reached     = false;

    QDate date_start;
    QTime time_start;

    QDate date_end;
    QTime time_end;

    QDateTime datetime_start;
    QDateTime datetime_end;

    int64_t jday_start;
    int64_t jday_end;

    const auto setTime = [] (QTime &t, const std::string &time_str)
    {
        const auto start_hour = time_str.substr(0, 2);
        const auto start_min  = time_str.substr(3, 2);

        t = QTime(std::stoi(start_hour), std::stoi(start_min));
    };

    bool needs_change           = true;
    args.w->temp_needs_change   = &needs_change;

    while (!args.w->quit)
    {
        sleep_for(2.5s);

        args.w->temp_cv->wait(lock, [&]{ return args.w->run_temp_thread; });

        if(needs_change)
        {
            jday_start = QDate::currentDate().toJulianDay();
            setTime(time_start, cfg["time_start"]);

            jday_end = jday_start + 1;
            setTime(time_end, cfg["time_end"]);
        }

        date_start      = QDate::fromJulianDay(jday_start);
        datetime_start  = QDateTime(date_start, time_start);

        date_end        = QDate::fromJulianDay(jday_end);
        datetime_end    = QDateTime(date_end, time_end);

        start_date_reached = QDateTime::currentDateTime() > datetime_start;
        end_date_reached   = QDateTime::currentDateTime() > datetime_end;

        LOGI << "Start reached: " << start_date_reached << " (" << datetime_start.toString() << ')';
        LOGI << "End   reached: "   << end_date_reached << " (" << datetime_end.toString() << ')';
        LOGI << "State: " << state << " | " << needs_change;

        int temp_target = cfg["temp_target"];

        if(start_date_reached)
        {
            state = LOW_TEMP;
        }
        else
        {
            temp_target = cfg["temp_initial"];

            if(state == LOW_TEMP)
            {
                if(jday_start == QDate::currentDate().toJulianDay())
                {
                    state = HIGH_TEMP;
                }
                else continue;
            }
            else if(!end_date_reached && !needs_change) continue;
        }

        cfg["temp_state"] = state;

        int target_step = kelvinToStep(temp_target);

        LOGI << "Adjusting temperature...";

        needs_change = false;

        while (args.w->run_temp_thread)
        {
            int step = cfg["temp_step"];

            if (step < target_step)
            {
                ++step;
            }
            else if(step > target_step)
            {
                --step;
            }
            else
            {
                int64_t tomorrow_jday = QDate::currentDate().addDays(1).toJulianDay();

                if(state == LOW_TEMP)
                {
                    jday_start = tomorrow_jday;
                }
                else if(state == HIGH_TEMP)
                {
                    jday_end = tomorrow_jday;
                }

                cfg["temp_state"] = state;

                break;
            }

            if(args.w->quit) break;

            if constexpr (os == OS::Windows)
            {
                setGDIGamma(scr_br, step);
            }
#ifndef _WIN32
            else { args.x11->setXF86Gamma(scr_br, step); }
#endif

            args.w->setTempSlider(step);

            cfg["temp_step"] = step;

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
