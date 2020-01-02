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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

// Reflects the current screen brightness
int scr_br = default_brightness;

#ifndef _WIN32
// To be used in unix signal handler
static bool *quit_ptr;
static convar *brcv_ptr;
static convar *tempcv_ptr;
#endif

struct Args
{
	convar	adjustbr_cv;
	convar	temp_cv;

#ifndef _WIN32
	X11 *x11 {};
#endif

	MainWindow *w {};

	int target_br	= 0;
	int img_delta	= 0;

	bool br_needs_change = false;
};

void adjustBrightness(Args &args)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	std::mutex m;

	while(true)
	{
		{
			std::unique_lock<std::mutex> lock(m);

			args.adjustbr_cv.wait(lock, [&]
			{
				return args.br_needs_change || args.w->quit;
			});
		}

		args.br_needs_change = false;

		if(args.w->quit) break;

		int sleeptime = (100 - args.img_delta / 4) / cfg["speed"].get<int>();
		args.img_delta = 0;

		int add = 0;

		// We check if brightness already equals target in the calling thread
		if (scr_br < args.target_br) {
			add = 1;
			sleeptime /= 3;
		}
		else {
			add = -1;
		}

		while (!args.br_needs_change && args.w->run_ss_thread)
		{
			scr_br += add;

			if(args.w->quit) break;

			if constexpr (os == OS::Windows) {
				setGDIGamma(scr_br, cfg["temp_step"]);
			}
#ifndef _WIN32
			else { args.x11->setXF86Gamma(scr_br, cfg["temp_step"]); }
#endif

			args.w->updateBrLabel();

			if(scr_br == args.target_br) break;

			sleep_for(milliseconds(sleeptime));
		}
	}
}

void adjustTemperature(Args &args)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	const auto setTime = [] (QTime &t, const std::string &time_str)
	{
		const auto start_hour	= time_str.substr(0, 2);
		const auto start_min	= time_str.substr(3, 2);

		t = QTime(std::stoi(start_hour), std::stoi(start_min));
	};

	enum TempState {
		HIGH_TEMP,
		LOW_TEMP
	};

	bool start_date_reached;
	bool end_date_reached;

	bool force = false;
	args.w->force_temp_change = &force;

	QTime		time_start;
	QTime		time_end;
	QDateTime	datetime_start;
	QDateTime	datetime_end;

	int64_t		jday_start;
	int64_t		jday_end;

	const auto setDates = [&]
	{
		setTime(time_start, cfg["time_start"]);
		setTime(time_end, cfg["time_end"]);

		datetime_start	= QDateTime(QDate::fromJulianDay(jday_start), time_start);
		datetime_end	= QDateTime(QDate::fromJulianDay(jday_end), time_end);

		LOGD << "Start: " << datetime_start.toString();
		LOGD << "End:   " << datetime_end.toString();
	};

	const auto checkDates = [&]
	{
		const auto now = QDateTime::currentDateTime();

		start_date_reached	= now > datetime_start;
		end_date_reached	= now > datetime_end;

		LOGD << "Start reached: " << start_date_reached;
		LOGD << "End   reached: " << end_date_reached;
	};

	LOGD << "Initializing temp schedule";

	int64_t today = QDate::currentDate().toJulianDay();
	int64_t tomorrow = today + 1;

	jday_start = today;
	jday_end = tomorrow;

	setDates();
	checkDates();

	if(cfg["temp_state"] == LOW_TEMP && !start_date_reached)
	{
		LOGD << "Starting on low temp, but start date hasn't been reached. End time should be today.";
		jday_end = today;

		setDates();
		checkDates();
	}

	bool needs_change;

	if(args.w->run_temp_thread)
	{
		needs_change = true;
		args.temp_cv.notify_one();
	}

	convar clock_cv;
	std::mutex clock_m;

	std::thread clock ([&]
	{
		while(true)
		{
			{
				std::unique_lock<std::mutex> lk(clock_m);
				clock_cv.wait_until(lk, system_clock::now() + 60s, [&] () { return args.w->quit; });
			}

			if(args.w->quit) break;

			LOGD << "Clock tick";

			if(!args.w->run_temp_thread)
			{
				LOGD << "Autotemp disabled. Skipping date check";
				continue;
			}

			checkDates();

			if(start_date_reached || end_date_reached)
			{
				needs_change = true;
				args.temp_cv.notify_one();
			}
		}

		LOGD << "Clock stopped";
	});

	std::mutex m;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lk(m);

			args.temp_cv.wait(lk, [&]
			{
				return needs_change || force || args.w->quit;
			});
		}

		if(args.w->quit) break;

		needs_change = false;

		if(force)
		{
			setDates();
			checkDates();

			if(cfg["temp_state"] == LOW_TEMP && !start_date_reached && jday_end != today)
			{
				LOGD << "Forcing to high temp";
				cfg["temp_state"] = HIGH_TEMP;
			}

			force = false;
		}

		if(!cfg["auto_temp"]) continue;

		if(cfg["temp_state"] == HIGH_TEMP)
		{
			if(start_date_reached)
			{
				LOGD << "Start date reached.";
				cfg["temp_state"] = LOW_TEMP;
			}
		}
		else
		{
			if(start_date_reached && end_date_reached)
			{
				LOGD << "Start and end reached. Shifting dates.";

				jday_start++;
				jday_end++;
				setDates();
				checkDates();

				cfg["temp_state"] = HIGH_TEMP;
			}
			else if(jday_end == today && end_date_reached)
			{
				// We get here if the app was started on low temp
				// but the start date hasn't been reached

				LOGD << "End date reached. Shifting end date.";

				cfg["temp_state"] = HIGH_TEMP;

				jday_end++;
				setDates();
				checkDates();
			}
		}

		int cur_step = cfg["temp_step"];
		int target_temp;

		if(cfg["temp_state"] == HIGH_TEMP)
			target_temp = cfg["temp_high"];
		else
			target_temp = cfg["temp_low"];

		const int target_step = kelvinToStep(target_temp);

		int add = 0;

		if(cur_step == target_step)
		{
			continue;
		}

		if(cur_step < target_step)
		{
			LOGD << "Decreasing temp...";
			add = 1;
		}
		else
		{
			LOGD << "Increasing temp...";
			add = -1;
		}

		while (args.w->run_temp_thread && !force)
		{
			cur_step += add;

			args.w->setTempSlider(cur_step);

			if(cur_step == target_step)
			{
				LOGD << "Done!";
				break;
			}

			sleep_for(50ms);
		}
	}

	LOGD << "Notifying clock thread to quit";

	clock_cv.notify_one();
	clock.join();
}

void recordScreen(Args &args)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	LOGV << "recordScreen() start";

	int	prev_imgBr	= 0,
		prev_min	= 0,
		prev_max	= 0,
		prev_offset	= 0;

	bool force	= false;
	args.w->force	= &force;

#ifdef _WIN32
	const uint64_t	w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN),
			h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN),
			len = w * h * 4;

	LOGD << "Screen resolution: " << w << "*" << h;

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

	const auto getSnapshot = [&]
	{
		LOGV << "Taking screenshot";

#ifdef _WIN32
		if (useDXGI)
		{
			while (!dx.getDXGISnapshot(buf)) dx.restartDXGI();
		}
		else
		{
			getGDISnapshot(buf);
			sleep_for(milliseconds(cfg["polling_rate"]));
		}
#else
		args.x11->getX11Snapshot(buf);

		sleep_for(milliseconds(cfg["polling_rate"]));
#endif
	};

	std::mutex m;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(m);

			args.adjustbr_cv.wait(lock, [&]
			{
				return args.w->run_ss_thread || args.w->quit;
			});
		}

		if(args.w->quit) break;

		while(args.w->run_ss_thread && !args.w->quit)
		{
			getSnapshot();

			int img_br	= calcBrightness(buf);
			args.img_delta += abs(prev_imgBr - img_br);

			std::call_once(f, [&] { args.img_delta = 0; });

			if (args.img_delta > cfg["threshold"] || force)
			{
				int offset = cfg["offset"];

				args.target_br = default_brightness - img_br + offset;

				if (args.target_br > cfg["max_br"]) {
					args.target_br = cfg["max_br"];
				}
				else
				if (args.target_br < cfg["min_br"]) {
					args.target_br = cfg["min_br"];
				}

				if(args.target_br != scr_br)
				{
					LOGD << scr_br << " -> " << args.target_br << ", Δ: " << args.img_delta;

					args.br_needs_change = true;
					args.adjustbr_cv.notify_one();
				}
				else args.img_delta = 0;

				force = false;
			}

			if (cfg["min_br"] != prev_min || cfg["max_br"] != prev_max || cfg["offset"] != prev_offset)
			{
				force = true;
			}

			prev_imgBr	= img_br;
			prev_min	= cfg["min_br"];
			prev_max	= cfg["max_br"];
			prev_offset	= cfg["offset"];
		}
	}

	if constexpr (os == OS::Windows) {
		setGDIGamma(default_brightness, 0);
	}
#ifndef _WIN32
	else args.x11->setInitialGamma(args.w->set_previous_gamma);
#endif

	LOGD << "Notifying brightness thread to quit";

	args.br_needs_change = true;
	args.adjustbr_cv.notify_one();

	t1.join();

	LOGD << "Brightness thread joined";

	t2.join();

	LOGD << "Temp thread joined";

	QApplication::quit();
}

int main(int argc, char **argv)
{
	static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

	plog::init(plog::Severity(plog::debug), &console_appender);

	read();

	// Start with manual brightness setting, if auto brightness is disabled
	if(!cfg["auto_br"]) scr_br = cfg["brightness"];

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

	Args thr_args;

#ifdef _WIN32
	MainWindow wnd(nullptr, &threads_args.auto_cv, &threads_args.temp_cv);
#else
	MainWindow wnd(&x11, &thr_args.adjustbr_cv, &thr_args.temp_cv);

	thr_args.x11	= &x11;

	brcv_ptr	= &thr_args.adjustbr_cv;
	tempcv_ptr	= &thr_args.temp_cv;
	quit_ptr	= &wnd.quit;
#endif

	thr_args.w = &wnd;

	std::thread t(recordScreen, std::ref(thr_args));

	a.exec();
	t.join();

	LOGD << "Exiting";

	return EXIT_SUCCESS;
}

#ifndef _WIN32
void sig_handler(int signo)
{
	LOGD_IF(signo == SIGINT) << "SIGINT received";
	LOGD_IF(signo == SIGTERM) << "SIGTERM received";
	LOGD_IF(signo == SIGQUIT) << "SIGQUIT received";

	save();

	if(!quit_ptr || ! brcv_ptr || !tempcv_ptr) _exit(0);

	*quit_ptr = true;
	brcv_ptr->notify_all();
	tempcv_ptr->notify_one();
}
#endif
