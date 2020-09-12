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
#include <algorithm>
#include "mainwindow.h"

// Reflects the current screen brightness
int brt_step = brt_slider_steps;

#ifndef _WIN32
// Pointers for quitting normally in signal handler
static bool   *p_quit;
static convar *p_ss_cv;
static convar *p_temp_cv;
#endif

void adjustTemperature(convar &temp_cv, MainWindow &w)
{
	using namespace std::this_thread;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	QTime start_time;
	QTime end_time;

	bool force          = false;
	w.force_temp_change = &force;

	const auto setTime = [] (QTime &t, const std::string &time_str)
	{
		const auto start_hour = time_str.substr(0, 2);
		const auto start_min  = time_str.substr(3, 2);

		t = QTime(std::stoi(start_hour), std::stoi(start_min));
	};

	const auto resetInterval = [&]
	{
		setTime(start_time, cfg["time_start"]);
		setTime(end_time,   cfg["time_end"]);
	};

	const auto checkTime = [&]
	{
		QTime cur_time = QTime::currentTime();

		return (cur_time >= start_time) || (cur_time < end_time);
	};

	resetInterval();

	bool should_be_low = checkTime();
	bool needs_change  = cfg["auto_temp"];

	convar     clock_cv;
	std::mutex clock_mtx;
	std::mutex temp_mtx;

	std::thread clock ([&]
	{
		while(true)
		{
			{
				std::unique_lock<std::mutex> lk(clock_mtx);
				clock_cv.wait_until(lk, system_clock::now() + 60s, [&] { return w.quit; });
			}

			if(w.quit) break;

			if(!cfg["auto_temp"]) continue;

			{
				std::lock_guard<std::mutex> lock(temp_mtx);

				should_be_low = checkTime();
				needs_change  = true; // @TODO: Should be false if the state hasn't changed
			}

			temp_cv.notify_one();
		}
	});

	/* We always need to catch up temperature a bit when:
	- the clock above checks the time a bit after the start time passes
	- we wake up from suspend
	- we modify start time, temperature targets or adaptation speed */
	bool already_catched_up = false;

	while (true)
	{
		// Lock ------------------------------------------------------------------------------------------

		{
			std::unique_lock<std::mutex> lock(temp_mtx);

			temp_cv.wait(lock, [&]
			{
				return needs_change || already_catched_up || force || w.quit;
			});

			if(w.quit) break;

			if(force)
			{
				resetInterval();
				should_be_low = checkTime();
				force         = false;

				already_catched_up = false;
			}

			needs_change = false;
		}

		if(!cfg["auto_temp"]) continue;

		// -----------------------------------------------------------------------------------------------

		int   target_temp; // Temperature target in Kelvin
		double duration_s; // Seconds it takes to reach it

		const double temp_speed_s = cfg["temp_speed"].get<double>() * 60;

		if(should_be_low) {

			const int secs_since_start = start_time.secsTo(QTime::currentTime());

			if(already_catched_up) {
				target_temp = cfg["temp_low"];
				duration_s  = temp_speed_s - secs_since_start;
			}
			else {
				target_temp = remap(secs_since_start, temp_speed_s, 0, cfg["temp_low"], cfg["temp_high"]);
				duration_s  = 2;

				if(target_temp < cfg["temp_low"]) { target_temp = cfg["temp_low"]; } // idk why this happens
			}
		}
		else
		{
			target_temp = cfg["temp_high"];
			duration_s  = 2;
		}

		if(duration_s < 2) { duration_s = 2; }

		LOGD << "Duration: " << duration_s / 60 << " min";

		int cur_step    = cfg["temp_step"];
		int target_step = int(remap(target_temp, min_temp_kelvin, max_temp_kelvin, temp_slider_steps, 0));

		if(cur_step == target_step)
		{
			LOGD << "Temp already at target (" << target_temp << " K)";

			already_catched_up = false;

			continue;
		}

		LOGD << "Temp target: " << target_temp << " K";

		const int    FPS        = cfg["temp_fps"];
		const double iterations = FPS * duration_s;
		const double time_incr  = duration_s / iterations;
		const int    distance   = target_step - cur_step;

		double time = 0;

		LOGD << "(" << cur_step << "->" << target_step << ')';

		while (cfg["temp_step"] != target_step && cfg["auto_temp"])
		{
			if(force || w.quit) break;

			time += time_incr;
			cfg["temp_step"] = int(easeInOutQuad(time, cur_step, distance, duration_s));

			w.setTempSlider(cfg["temp_step"]);

			sleep_for(milliseconds(1000 / FPS));
		}

		already_catched_up = cur_step != cfg["target_step"];

		LOGD << "(" << cur_step << "->" << target_step << ") done";
	}

	LOGV << "Notifying clock thread";

	clock_cv.notify_one();
	clock.join();

	LOGV << "Clock thread joined";
}

struct Args
{
	convar br_cv;
	std::mutex br_mtx;

#ifndef _WIN32
	X11 *x11 {};
#endif

	int img_br = 0;
	bool br_needs_change = false;
};

void adjustBrightness(Args &args, MainWindow &w)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	while(true)
	{
		int img_br;

		{
			std::unique_lock<std::mutex> lock(args.br_mtx);

			args.br_cv.wait(lock, [&]
			{
				return args.br_needs_change;
			});

			if(w.quit) break;

			args.br_needs_change = false;

			img_br = args.img_br;
		}

		int target = brt_slider_steps - int(remap(img_br, 0, 255, 0, brt_slider_steps)) + cfg["offset"].get<int>();
		target = std::clamp(target, cfg["min_br"].get<int>(), cfg["max_br"].get<int>());

		if (target == brt_step)
		{
			LOGD << "Brt already at target (" << target << ')';
			continue;
		}

		const int start = brt_step;
		const int end   = target;
		double duration_s = cfg["speed"];

		const int FPS           = cfg["brt_fps"];
		const double iterations = FPS * duration_s;
		const int distance      = end - start;
		const double time_incr  = duration_s / iterations;

		double time = 0;

		LOGD << "(" << start << "->" << end << ')';

		while (brt_step != target && !args.br_needs_change && cfg["auto_br"] && !w.quit)
		{
			time += time_incr;

			brt_step = std::round(easeOutExpo(time, start, distance, duration_s));

			w.setBrtSlider(brt_step);

			sleep_for(milliseconds(1000 / FPS));
		}

		LOGD << "(" << start << "->" << end << ") done";
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

	args.x11->setXF86Gamma(brt_step, cfg["temp_step"]);
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

			const int img_br = calcBrightness(buf);
			img_delta += abs(prev_img_br - img_br);

			if (img_delta > cfg["threshold"] || force)
			{
				img_delta = 0;
				force = false;

				{
					std::lock_guard lock (args.br_mtx);

					args.img_br = img_br;
					args.br_needs_change = true;
				}

				args.br_cv.notify_one();
			}

			if (cfg["min_br"] != prev_min || cfg["max_br"] != prev_max || cfg["offset"] != prev_offset)
			{
				force = true;
			}

			prev_img_br = img_br;
			prev_min    = cfg["min_br"];
			prev_max    = cfg["max_br"];
			prev_offset = cfg["offset"];
		}

		buf.clear();
		buf.shrink_to_fit();
	}

	LOGV << "Exited screenshot loop. Notifying adjustBrightness";

	{
		std::lock_guard<std::mutex> lock (args.br_mtx);
		args.br_needs_change = true;
	}

	args.br_cv.notify_one();

	br_thr.join();

	LOGV << "adjustBrightness joined";

	LOGV << "Notifying QApplication";

	QApplication::quit();
}

void sig_handler(int signo);

void init()
{
	static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

	plog::init(plog::Severity(plog::debug), &console_appender);

	read();

	if(!cfg["auto_br"].get<bool>())
	{
		// Start with manual brightness setting, if auto brightness is disabled
		LOGV << "Autobrt OFF. Setting manual brt step.";
		brt_step = cfg["brightness"];
	}

	if(cfg["auto_temp"].get<bool>())
	{
		LOGV << "Autotemp ON. Starting from step 0."; // To allow smooth transition
		cfg["temp_step"] = 0;
	}

	plog::get()->addAppender(&file_appender);
	plog::get()->setMaxSeverity(plog::Severity(cfg["log_lvl"]));

#ifndef _WIN32
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGTERM, sig_handler);
#else
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
#endif
}

int main(int argc, char **argv)
{
	init();

	QApplication a(argc, argv);

	convar ss_cv;
	convar temp_cv;
	Args thr_args;

#ifdef _WIN32
	MainWindow wnd(nullptr, &ss_cv, &temp_cv);
#else
	X11 x11;

	MainWindow wnd(&x11, &ss_cv, &temp_cv);

	thr_args.x11 = &x11;
	p_quit = &wnd.quit;
	p_ss_cv = &ss_cv;
	p_temp_cv = &temp_cv;
#endif

	std::thread temp_thr(adjustTemperature, std::ref(temp_cv), std::ref(wnd));
	std::thread ss_thr(recordScreen, std::ref(thr_args), std::ref(ss_cv), std::ref(wnd));

	a.exec();

	LOGV << "QApplication joined";

	temp_thr.join();

	LOGV << "adjustTemperature joined";

	ss_thr.join();

	LOGV << "recordScreen joined";

	if constexpr (os == OS::Windows) {
		setGDIGamma(brt_slider_steps, 0);
	}
#ifndef _WIN32
	else x11.setInitialGamma(wnd.set_previous_gamma);
#endif

	LOGV << "Exiting";

	return EXIT_SUCCESS;
}

#ifndef _WIN32
void sig_handler(int signo)
{
	LOGD_IF(signo == SIGINT) << "SIGINT received";
	LOGD_IF(signo == SIGTERM) << "SIGTERM received";
	LOGD_IF(signo == SIGQUIT) << "SIGQUIT received";

	write();

	if(!p_quit || ! p_ss_cv || !p_temp_cv) _exit(0);

	*p_quit = true;
	p_ss_cv->notify_one();
	p_temp_cv->notify_one();
}
#endif
