#ifndef GAMMACTL_H
#define GAMMACTL_H

class MainWindow;
class ScreenCtl;

#include "defs.h"

class GammaCtl
{
public:
	ScreenCtl *screen;

	GammaCtl(ScreenCtl*);

	void exec(MainWindow *wnd);
	void recordScreen();
	void adjustTemperature();
	void notify_ss();
	void notify_temp(bool force = false);
	void close();

private:
	MainWindow *wnd;
	convar ss_cv;
	convar br_cv;
	convar temp_cv;
	int img_br = 0;

	std::mutex br_mtx;

	void adjustBrightness();
	bool br_needs_change   = false;
	bool force_temp_change = false;
	bool quit              = false;
};

#endif // GAMMACTL_H
