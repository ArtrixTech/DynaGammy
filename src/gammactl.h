#ifndef GAMMACTL_H
#define GAMMACTL_H

#include <vector>
#include <thread>

#include "defs.h"

#include "mainwindow.h"
#include "screenctl.h"

class GammaCtl : public ScreenCtl
{
public:
	GammaCtl();

	void setWindow(MainWindow*);
	void start();
	void stop();

	void notify_ss();
	void notify_temp(bool force = false);
private:
	MainWindow *wnd;

	convar ss_cv;
	convar temp_cv;
	convar reapply_cv;

	int brt_step;
	int temp_step;

	int ss_brightness = 0;

	std::mutex br_mtx;
	void captureScreen();
	int  calcBrightness(const std::vector<uint8_t> &buf, int bpp = 4, int skip_mult = 1);
	void adjustBrightness(convar &br_cv);
	void adjustTemperature();
	void reapplyGamma();
	void notify_all_threads();
	bool br_needs_change   = false;
	bool force_temp_change = false;
	bool quit              = false;

	std::vector<std::thread> threads;
};

#endif // GAMMACTL_H
