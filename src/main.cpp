/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
	#include "dxgidupl.h"
	#pragma comment(lib, "gdi32.lib")
	#pragma comment(lib, "user32.lib")
	#pragma comment(lib, "DXGI.lib")
	#pragma comment(lib, "D3D11.lib")
	#pragma comment(lib, "Advapi32.lib")
#else
	#include <signal.h>
#endif

#include "cfg.h"
#include "utils.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <QApplication>
#include <QTime>
#include "mainwindow.h"

// Reflects the current screen brightness
int scr_br = default_brightness;

#ifndef _WIN32
// Pointers for quitting normally in signal handler
static bool	*p_quit;
static convar	*p_ss_cv;
static convar	*p_temp_cv;
#endif

void adjustTemperature(convar &temp_cv, MainWindow &w)
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
	w.force_temp_change = &force;

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

		LOGV << "Start reached: " << start_date_reached;
		LOGV << "End   reached: " << end_date_reached;
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

	std::mutex temp_mtx;

	bool needs_change = cfg["auto_temp"];

	convar		clock_cv;
	std::mutex	clock_mtx;

	std::thread clock ([&]
	{
		while(true)
		{
			{
				std::unique_lock<std::mutex> lk(clock_mtx);
				clock_cv.wait_until(lk, system_clock::now() + 60s, [&] { return w.quit; });
			}

			if(w.quit) break;

			if(!cfg["auto_temp"])
			{
				LOGD << "Skipping date check (adaptive temp disabled)";
				continue;
			}

			LOGV << "Checking time";

			checkDates();

			{
				std::lock_guard<std::mutex> lock(temp_mtx);

				bool is_high = cfg["temp_state"] == HIGH_TEMP;
				needs_change = (start_date_reached && is_high) || (end_date_reached && !is_high);
			}

			temp_cv.notify_one();
		}
	});

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(temp_mtx);

			temp_cv.wait(lock, [&]
			{
				return needs_change || force || w.quit;
			});

			if(w.quit)
			{
				break;
			}
			if(force)
			{
				setDates();
				checkDates();

				if(cfg["temp_state"] == LOW_TEMP && !start_date_reached && jday_end != today)
				{
					LOGD << "Forcing high temp";
					cfg["temp_state"] = HIGH_TEMP;
				}

				force = false;
			}
			else
			{
				needs_change = false;
			}
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

		const int target_step = kelvinToStep(cfg["temp_state"] == HIGH_TEMP ? cfg["temp_high"] : cfg["temp_low"]);

		int cur_step = cfg["temp_step"];
		int add = 0;

		if(target_step == cur_step)
		{
			LOGD << "Temperature is already at target.";
			continue;
		}
		else if(target_step > cur_step)
		{
			LOGD << "Decreasing temp...";
			add = 1;
		}
		else
		{
			LOGD << "Increasing temp...";
			add = -1;
		}

		while (cfg["auto_temp"])
		{
			if(w.quit || force) break;

			cur_step += add;

			w.setTempSlider(cur_step);

			if(cur_step == target_step)
			{
				LOGD << "Done!";
				break;
			}

			sleep_for(50ms);
		}
	}

	LOGD << "Notifying clock thread";

	clock_cv.notify_one();
	clock.join();

	LOGD << "Clock thread joined";
}

struct Args
{
	convar br_cv;
	std::mutex br_mtx;

#ifndef _WIN32
	X11 *x11 {};
#endif

	int img_br = 0;
	int img_delta = 0;
	bool br_needs_change = false;
};

void adjustBrightness(Args &args, MainWindow &w)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	while(true)
	{
		int img_br;
		int delta;

		{
			std::unique_lock<std::mutex> lock(args.br_mtx);

			args.br_cv.wait(lock, [&]
			{
				return args.br_needs_change;
			});

			if(w.quit) break;

			args.br_needs_change = false;

			img_br = args.img_br;
			delta = args.img_delta;
			args.img_delta = 0;
		}

		int target = default_brightness - img_br + cfg["offset"].get<int>();

		if (target > cfg["max_br"]) {
			target = cfg["max_br"];
		}
		else
		if (target < cfg["min_br"]) {
			target = cfg["min_br"];
		}

		if (target == scr_br)
		{
			LOGD << "Brightness is already at target.";
			continue;
		}

		int sleeptime = (100 - delta / 4) / cfg["speed"].get<int>();
		int add = 0;

		if (target > scr_br)
		{
			add = 1;
			sleeptime /= 3;
		}
		else
		{
			add = -1;
		}

		LOGD << scr_br << "->" << target;

		while (!args.br_needs_change && cfg["auto_br"])
		{
			scr_br += add;

			if(w.quit) break;

			if constexpr (os == OS::Windows) {
				setGDIGamma(scr_br, cfg["temp_step"]);
			}
#ifndef _WIN32
			else args.x11->setXF86Gamma(scr_br, cfg["temp_step"]);
#endif

			w.updateBrLabel();

			if(scr_br == target) break;

			sleep_for(milliseconds(sleeptime));
		}
	}
}

void recordScreen(Args &args, convar &ss_cv, MainWindow &w)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	LOGV << "recordScreen() start";

#ifdef _WIN32
	const uint64_t width	= GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
	const uint64_t height	= GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
	const uint64_t len	= width * height * 4;

	LOGD << "Screen resolution: " << width << '*' << height;

	DXGIDupl dx;

	bool useDXGI = dx.initDXGI();

	if (!useDXGI)
	{
		LOGE << "DXGI initialization failed. Using GDI instead";
		w.setPollingRange(1000, 5000);
	}
#else
	const uint64_t screen_res = args.x11->getWidth() * args.x11->getHeight();
	const uint64_t len = screen_res * 4;

	args.x11->setXF86Gamma(scr_br, cfg["temp_step"]);
#endif

	LOGD << "Buffer size: " << len;

	// Buffer to store screen pixels
	std::vector<uint8_t> buf;

	std::thread br_thr(adjustBrightness, std::ref(args), std::ref(w));

	const auto getSnapshot = [&] (std::vector<uint8_t> &buf)
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

	int img_delta = 0;

	bool force = false;

	int
	prev_img_br	= 0,
	prev_min	= 0,
	prev_max	= 0,
	prev_offset	= 0;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(m);

			ss_cv.wait(lock, [&]
			{
				return cfg["auto_br"] || w.quit;
			});
		}

		if(w.quit)
		{
			break;
		}

		if(cfg["auto_br"])
		{
			buf.resize(len);
			force = true;
		}
		else
		{
			buf.clear();
			buf.shrink_to_fit();
			continue;
		}

		while(cfg["auto_br"] && !w.quit)
		{
			getSnapshot(buf);

			int img_br = calcBrightness(buf);
			img_delta += abs(prev_img_br - img_br);

			if (img_delta > cfg["threshold"] || force)
			{
				force = false;

				{
					std::lock_guard lock (args.br_mtx);

					args.img_br = img_br;
					args.img_delta = img_delta;
					args.br_needs_change = true;
				}

				img_delta = 0;

				args.br_cv.notify_one();
			}

			if (cfg["min_br"] != prev_min || cfg["max_br"] != prev_max || cfg["offset"] != prev_offset)
			{
				force = true;
			}

			prev_img_br	= img_br;
			prev_min	= cfg["min_br"];
			prev_max	= cfg["max_br"];
			prev_offset	= cfg["offset"];
		}

		buf.clear();
		buf.shrink_to_fit();
	}

	LOGD << "Exited screenshot loop. Notifying adjustBrightness";

	{
		std::lock_guard<std::mutex> lock (args.br_mtx);
		args.br_needs_change = true;
	}

	args.br_cv.notify_one();

	br_thr.join();

	LOGD << "adjustBrightness joined";

	LOGD << "Notifying QApplication";

	QApplication::quit();
}

void sig_handler(int signo);

int main(int argc, char **argv)
{
	static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

	plog::init(plog::Severity(plog::debug), &console_appender);

	read();

	// Start with manual brightness setting, if auto brightness is disabled
	if(!cfg["auto_br"]) scr_br = cfg["brightness"];

	plog::get()->addAppender(&file_appender);
	plog::get()->setMaxSeverity(plog::Severity(cfg["log_lvl"]));

#ifdef _WIN32
	checkInstance();
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	if(cfg["log_lvl"] == plog::verbose)
	{
		FILE *f1, *f2, *f3;
		AllocConsole();
		freopen_s(&f1, "CONIN$", "r", stdin);
		freopen_s(&f2, "CONOUT$", "w", stdout);
		freopen_s(&f3, "CONOUT$", "w", stderr);
	}

	checkGammaRange();
#else
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGTERM, sig_handler);

	X11 x11;
#endif

	QApplication a(argc, argv);

	convar ss_cv;
	convar temp_cv;
	Args thr_args;

#ifdef _WIN32
	MainWindow wnd(nullptr, &ss_cv, &temp_cv);
#else
	MainWindow wnd(&x11, &ss_cv, &temp_cv);

	thr_args.x11 = &x11;
	p_quit = &wnd.quit;
	p_ss_cv = &ss_cv;
	p_temp_cv = &temp_cv;
#endif

	std::thread temp_thr(adjustTemperature, std::ref(temp_cv), std::ref(wnd));
	std::thread ss_thr(recordScreen, std::ref(thr_args), std::ref(ss_cv), std::ref(wnd));

	a.exec();

	LOGD << "QApplication joined";

	temp_thr.join();

	LOGD << "adjustTemperature joined";

	ss_thr.join();

	LOGD << "recordScreen joined";

	if constexpr (os == OS::Windows) {
		setGDIGamma(default_brightness, 0);
	}
#ifndef _WIN32
	else x11.setInitialGamma(wnd.set_previous_gamma);
#endif

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

	if(!p_quit || ! p_ss_cv || !p_temp_cv) _exit(0);

	*p_quit = true;
	p_ss_cv->notify_one();
	p_temp_cv->notify_one();
}
#endif
