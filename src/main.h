/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef MAIN_H
#define MAIN_H

//#define dbgthr
//#define dbgbr

extern int scr_br;
extern int polling_rate_min, polling_rate_max;

#ifndef _WIN32
void sig_handler(int signo);
#endif

#endif // MAIN_H
