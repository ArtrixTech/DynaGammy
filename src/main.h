/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef MAIN_H
#define MAIN_H

//#define dbg
//#define dbgthr
//#define dbgcfg
//#define dbgbr

constexpr int default_brightness = 255;

constexpr int min_temp_kelvin = 2000;
constexpr int max_temp_kelvin = 6500;
constexpr int temp_steps = 46 * 3;

extern int scr_br;
extern int polling_rate_min, polling_rate_max;

#ifndef _WIN32
void sig_handler(int signo);
#endif

#endif // MAIN_H
