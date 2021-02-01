/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <QApplication>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include "cfg.h"
#include "utils.h"
#include "mainwindow.h"
#include "gammactl.h"

#ifndef _WIN32
#include <signal.h>
void sig_handler(int signo)
{
	LOGD_IF(signo == SIGINT) << "SIGINT received";
	LOGD_IF(signo == SIGTERM) << "SIGTERM received";
	LOGD_IF(signo == SIGQUIT) << "SIGQUIT received";
	QApplication::quit();
}
#endif

void init()
{
	static plog::RollingFileAppender<plog::TxtFormatter> f("gammylog.txt", 1024 * 1024 * 5, 1);
	static plog::ColorConsoleAppender<plog::TxtFormatter> c;
	plog::init(plog::Severity(plog::debug), &c);

	const auto logger = plog::get();
	logger->addAppender(&f);
	config::read();
	logger->setMaxSeverity(plog::Severity(cfg["log_level"]));

	if (alreadyRunning()) {
		LOGI << "Process already running";
		exit(1);
	}

#ifndef _WIN32
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGTERM, sig_handler);
#else
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

	return app.exec();
}
