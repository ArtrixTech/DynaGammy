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
	convar br_cv;
	convar temp_cv;

	int brt_step;
	int temp_step;

	int ss_brightness = 0;

	std::mutex br_mtx;
	void captureScreen();
	void adjustBrightness();
	void adjustTemperature();
	bool br_needs_change   = false;
	bool force_temp_change = false;
	bool quit              = false;

	std::vector<std::thread> threads;
};

#endif // GAMMACTL_H
