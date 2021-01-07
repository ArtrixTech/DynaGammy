/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef GAMMACTL_H
#define GAMMACTL_H

#include <vector>
#include <thread>
#include "defs.h"

#ifdef _WIN32
#include "dspctl-dxgi.h"
#else
#include "dspctl-xlib.h"
#undef Status
#undef Bool
#undef CursorShape
#undef None
#endif

#include "component.h"

class GammaCtl : public DspCtl, public Component
{
public:
	GammaCtl();

	void start();
	void stop();

	void notify_ss();
	void notify_temp(bool force = false);
private:
	void captureScreen();
	void adjustBrightness(convar &br_cv);
	void adjustTemperature();
	void reapplyGamma();
	void notify_all_threads();

	std::vector<std::thread> threads;
	convar ss_cv;
	convar temp_cv;
	convar reapply_cv;
	std::mutex brt_mtx;
	int ss_brightness = 0;
	bool br_needs_change   = false;
	bool force_temp_change = false;
	bool quit              = false;
};

#endif // GAMMACTL_H
