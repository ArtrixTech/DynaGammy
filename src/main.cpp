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

#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

#include "cfg.h"
#include "utils.h"
#include "mainwindow.h"
#include "gammactl.h"

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
	static plog::RollingFileAppender<plog::TxtFormatter> f("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> c;

	plog::init(plog::Severity(plog::debug), &c);

	config::read();

	plog::get()->addAppender(&f);
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
	MainWindow   wnd;
	GammaCtl     gmm;
	Mediator     m(&gmm, &wnd);

	p_wnd = &wnd;

	return app.exec();
}
