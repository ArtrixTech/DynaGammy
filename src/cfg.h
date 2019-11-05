/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef CFG_H
#define CFG_H

#include <array>

constexpr int cfg_count = 15;

constexpr std::array<const char*, cfg_count> cfg_str
{
    "minBrightness=",
    "maxBrightness=",
    "offset=",
    "temp=",
    "speed=",
    "threshold=",
    "updateRate=",
    "auto=",
    "toggleLimit=",
    "debug=",
    "autoTemp=",
    "tempStart=",
    "tempEnd=",
    "hourStart=",
    "hourEnd="
};

enum cfg {
    MinBr,
    MaxBr,
    Offset,
    Temp,
    Speed,
    Threshold,
    Polling_rate,
    isAuto,
    toggleLimit,
    Debug,
    isAutoTemp,
    TempStart,
    TempEnd,
    HourStart,
    HourEnd
};

auto getHomePath()          -> std::string;
auto getExecutablePath()    -> std::wstring;

void readConfig();
void saveConfig();

extern std::array<int, cfg_count> cfg;

#endif // CFG_H
