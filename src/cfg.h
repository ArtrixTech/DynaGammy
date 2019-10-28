#ifndef CFG_H
#define CFG_H

#endif // CFG_H

#include <array>

enum class OS
{
    Windows, Unix
};

#ifdef _WIN32
constexpr OS os = OS::Windows;
#else
constexpr OS os = OS::Unix;
#endif

constexpr int cfg_count = 10;

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
    "debug="
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
    Debug
};

auto getHomePath()          -> std::string;
auto getExecutablePath()    -> std::wstring;

void readConfig();
void saveConfig();

extern std::array<int, cfg_count> cfg;
