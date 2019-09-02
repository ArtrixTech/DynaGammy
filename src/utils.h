/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <array>

constexpr int cfg_count = 9;

constexpr std::array<const char*, cfg_count> cfg_str = {
    "minBrightness=",
    "maxBrightness=",
    "offset=",
    "temp=",
    "speed=",
    "threshold=",
    "updateRate=",
    "auto=",
    "toggleLimit="
};

extern std::array<int, cfg_count> cfg;

enum {
    MinBr,
    MaxBr,
    Offset,
    Temp,
    Speed,
    Threshold,
    Polling_rate,
    isAuto,
    toggleLimit
};

int calcBrightness(uint8_t* buf, uint64_t screen_res);

void readConfig();
void updateConfig();

#ifdef _WIN32

void getGDISnapshot(uint8_t* buf);
void setGDIBrightness(unsigned short brightness, int temp);

void checkGammaRange();
void toggleRegkey(bool);

std::wstring getExecutablePath(bool add_cfg);
#else
std::string getHomePath(bool add_cfg);
#endif

#endif // UTILS_H
