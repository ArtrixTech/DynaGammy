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

#include <QApplication>

#include "cfg.h"
#include "utils.h"
#include "defs.h"

#include "screenctl.h"
#include "mainwindow.h"
#include "gammactl.h"

// Reflects the current screen brightness
int brt_step = brt_slider_steps;

#ifndef _WIN32
static MainWindow *p_wnd;

void sig_handler(int signo)
{
	LOGD_IF(signo == SIGINT) << "SIGINT received";
	LOGD_IF(signo == SIGTERM) << "SIGTERM received";
	LOGD_IF(signo == SIGQUIT) << "SIGQUIT received";

	if (!p_wnd)
		_exit(0);

	p_wnd->quit(p_wnd->prev_gamma);
}
#endif

void init()
{	
	static plog::RollingFileAppender<plog::TxtFormatter> file_appender("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;

	plog::init(plog::Severity(plog::debug), &console_appender);

	config::read();

	if (!cfg["auto_br"].get<bool>()) {
		// Start with manual brightness setting, if auto brightness is disabled
		LOGV << "Autobrt OFF. Setting manual brt step.";
		brt_step = cfg["brightness"];
	}

	if (cfg["auto_temp"].get<bool>()) {
		// To allow smooth transition
		LOGV << "Autotemp ON. Starting from step 0.";
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

	if (cfg["log_lvl"] == plog::verbose) {
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
	if (argc > 1) {
		if (strcmp(argv[1], "-v") == 0) {
			std::cout << g_app_version << '\n';
			exit(0);
		}
	}

	init();

	QApplication app(argc, argv);
	ScreenCtl    screen;
	GammaCtl     gammactl(&screen);
	MainWindow   wnd(&gammactl);

	p_wnd = &wnd;

	gammactl.exec(&wnd);
	app.exec();

	screen.setInitialGamma(wnd.prev_gamma);
	return 0;
}
