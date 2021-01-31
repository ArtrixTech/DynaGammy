/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QTime>
#include <thread>
#include "gammactl.h"
#include "defs.h"
#include "utils.h"
#include "cfg.h"
#include "mediator.h"

GammaCtl::GammaCtl()
{
	// If auto brightness is on, start at max brightness
	if (cfg["brt_auto"].get<bool>())
		cfg["brt_step"] = brt_steps_max;

	// If auto temp is on, start at max temp for a smooth transition
	if (cfg["temp_auto"].get<bool>())
		cfg["temp_step"] = 0;

	setGamma(cfg["brt_step"].get<int>(), cfg["temp_step"].get<int>());
}

void GammaCtl::start()
{
	LOGD << "Starting gamma control";

	if (!threads.empty())
		return;

	threads.emplace_back(std::thread([this] { adjustTemperature(); }));
	threads.emplace_back(std::thread([this] { captureScreen(); }));
	threads.emplace_back(std::thread([this] { reapplyGamma(); }));
}

void GammaCtl::stop()
{
	LOGD << "Stopping gamma control";

	if (threads.empty())
		return;

	quit = true;
	notify_all_threads();

	for (auto &t : threads)
		t.join();

	threads.clear();
}

void GammaCtl::notify_temp(bool force)
{
	force_temp_change = force;
	temp_cv.notify_one();
}

void GammaCtl::notify_ss()
{
	ss_cv.notify_one();
}

void GammaCtl::notify_all_threads()
{
	temp_cv.notify_one();
	ss_cv.notify_one();
	reapply_cv.notify_one();
}

void GammaCtl::reapplyGamma()
{
	using namespace std::this_thread;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	std::mutex mtx;

	while (true) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			reapply_cv.wait_until(lock, system_clock::now() + 5s, [&] {
				return quit;
			});
		}

		if (quit)
			break;

		setGamma(cfg["brt_step"], cfg["temp_step"]);
	}
}

void GammaCtl::captureScreen()
{
	LOGV << "captureScreen() start";

	convar      brt_cv;
	std::thread brt_thr([&] { adjustBrightness(brt_cv); });
	std::mutex  m;

	int img_delta = 0;
	bool force    = false;

	int
	prev_img_br = 0,
	prev_min    = 0,
	prev_max    = 0,
	prev_offset = 0;

	while (true) {
		{
			std::unique_lock<std::mutex> lock(m);

			ss_cv.wait(lock, [&] {
				return cfg["brt_auto"].get<bool>() || quit;
			});
		}

		if (quit)
			break;

		if (cfg["brt_auto"].get<bool>())
			force = true;
		else
			continue;

		while (cfg["brt_auto"].get<bool>() && !quit) {

			const int img_br = getScreenBrightness();
			img_delta += abs(prev_img_br - img_br);

			if (img_delta > cfg["brt_threshold"].get<int>() || force) {

				img_delta = 0;
				force = false;

				{
					std::lock_guard lock(brt_mtx);
					this->ss_brightness = img_br;
					br_needs_change = true;
				}

				brt_cv.notify_one();
			}

			if (cfg["brt_min"] != prev_min || cfg["brt_max"] != prev_max || cfg["brt_offset"] != prev_offset)
				force = true;

			prev_img_br = img_br;
			prev_min    = cfg["brt_min"];
			prev_max    = cfg["brt_max"];
			prev_offset = cfg["brt_offset"];

			// On Windows, we sleep in getScreenBrightness()
			if constexpr (!windows) {
				std::this_thread::sleep_for(std::chrono::milliseconds(cfg["brt_polling_rate"].get<int>()));
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock (brt_mtx);
		br_needs_change = true;
	}

	brt_cv.notify_one();
	brt_thr.join();
}

void GammaCtl::adjustBrightness(convar &brt_cv)
{
	using namespace std::this_thread;
	using namespace std::chrono;

	while (true) {
		int img_br;

		{
			std::unique_lock<std::mutex> lock(brt_mtx);

			brt_cv.wait(lock, [&] {
				return br_needs_change;
			});

			if (quit)
				break;

			br_needs_change = false;
			img_br = this->ss_brightness;
		}

		const int cur_step = cfg["brt_step"];
		const int tmp = brt_steps_max
		                - int(remap(img_br, 0, 255, 0, brt_steps_max))
		                + int(remap(cfg["brt_offset"].get<int>(), 0, brt_steps_max, 0, cfg["brt_max"].get<int>()));
		const int target_step = std::clamp(tmp, cfg["brt_min"].get<int>(), cfg["brt_max"].get<int>());

		if (cur_step == target_step) {
			LOGV << "Brt already at target (" << target_step << ')';
			continue;
		}

		double time             = 0;
		const int FPS           = cfg["brt_fps"];
		const double slice      = 1. / FPS;
		const double duration_s = cfg["brt_speed"].get<double>() / 1000;
		const int diff          = target_step - cur_step;

		while (cfg["brt_step"].get<int>() != target_step) {

			if (br_needs_change || !cfg["brt_auto"].get<bool>() || quit)
				break;

			time += slice;
			cfg["brt_step"] = int(std::round(easeOutExpo(time, cur_step, diff, duration_s)));

			setGamma(cfg["brt_step"], cfg["temp_step"]);
			mediator->notify(this, BRT_CHANGED);
			sleep_for(milliseconds(1000 / FPS));
		}
	}
}

/**
 * The temperature is adjusted in two steps.
 * The first one is for quickly catching up to the proper temperature when:
 * - time is checked sometime after the start time
 * - the system wakes up
 * - temperature settings change
 */
void GammaCtl::adjustTemperature()
{
	using namespace std::this_thread;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	QTime start_time;
	QTime end_time;

	const auto setTime = [] (QTime &t, const std::string &time_str) {
		const auto start_h = time_str.substr(0, 2);
		const auto start_m = time_str.substr(3, 2);
		t = QTime(std::stoi(start_h), std::stoi(start_m));
	};

	const auto updateInterval = [&] {
		std::string t_start = cfg["temp_sunset"];
		const int h = std::stoi(t_start.substr(0, 2));
		const int m = std::stoi(t_start.substr(3, 2));

		const int adapt_time_s = cfg["temp_speed"].get<double>() * 60;
		const QTime adapted_start = QTime(h, m).addSecs(-adapt_time_s);

		setTime(start_time, adapted_start.toString().toStdString());
		setTime(end_time, cfg["temp_sunrise"]);
	};

	updateInterval();

	bool needs_change = cfg["temp_auto"];

	convar     clock_cv;
	std::mutex clock_mtx;
	std::mutex temp_mtx;

	std::thread clock ([&] {
		while (true) {
			{
				std::unique_lock<std::mutex> lk(clock_mtx);
				clock_cv.wait_until(lk, system_clock::now() + 60s, [&] {
					return quit;
				});
			}

			if (quit)
				break;

			if (!cfg["temp_auto"])
				continue;

			{
				std::lock_guard<std::mutex> lock(temp_mtx);
				needs_change = true; // @TODO: Should be false if the state hasn't changed
			}

			temp_cv.notify_one();
		}
	});

	bool first_step_done = false;

	while (true) {
		{
			std::unique_lock<std::mutex> lock(temp_mtx);

			temp_cv.wait(lock, [&] {
				return needs_change || first_step_done || force_temp_change || quit;
			});

			if (quit)
				break;

			if (force_temp_change) {
				updateInterval();
				force_temp_change = false;
				first_step_done = false;
			}

			needs_change = false;
		}

		if (!cfg["temp_auto"])
			continue;


		int    target_temp = cfg["temp_low"]; // Temperature target in Kelvin
		double duration_s  = 2;               // Seconds it takes to reach it

		const double    adapt_time_s = cfg["temp_speed"].get<double>() * 60;
		const QDateTime cur_datetime = QDateTime::currentDateTime();
		const QTime     cur_time     = cur_datetime.time();

		if ((cur_time >= start_time) || (cur_time < end_time)) {

			QDateTime start_datetime(cur_datetime.date(), start_time);

			/* If we are earlier than both sunset and sunrise times
			 * we need to count from yesterday. */
			if (cur_time < end_time)
				start_datetime = start_datetime.addDays(-1);

			int secs_from_start = start_datetime.secsTo(cur_datetime);

			LOGV << "secs_from_start: " << secs_from_start << " adapt_time_s: " << adapt_time_s;

			if (secs_from_start > adapt_time_s)
				secs_from_start = adapt_time_s;

			if (!first_step_done) {
				target_temp = remap(secs_from_start, 0, adapt_time_s, cfg["temp_high"], cfg["temp_low"]);
			} else {
				duration_s = adapt_time_s - secs_from_start;
				if (duration_s < 2)
					duration_s = 2;
			}
		} else {
			target_temp = cfg["temp_high"];
		}

		LOGV << "Temp duration: " << duration_s / 60 << " min";

		int cur_step    = cfg["temp_step"];
		int target_step = int(remap(target_temp, temp_k_max, temp_k_min, temp_steps_max, 0));

		if (cur_step == target_step) {
			first_step_done = false;
			continue;
		}

		double time        = 0;
		const int FPS      = cfg["temp_fps"];
		const double slice = 1. / FPS;
		const int diff     = target_step - cur_step;

		while (cfg["temp_step"].get<int>() != target_step) {

			if (force_temp_change || !cfg["temp_auto"].get<bool>() || quit)
				break;

			time += slice;
			cfg["temp_step"] = int(easeInOutQuad(time, cur_step, diff, duration_s));

			setGamma(cfg["brt_step"], cfg["temp_step"]);
			mediator->notify(this, TEMP_CHANGED);
			sleep_for(milliseconds(1000 / FPS));
		}

		first_step_done = true;
	}

	clock_cv.notify_one();
	clock.join();
}
