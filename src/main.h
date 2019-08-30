#ifndef MAIN_H
#define MAIN_H

#define dbg

#include <string>
#include <array>
#include <condition_variable>
extern std::condition_variable pausethr;

constexpr unsigned char min_brightness_limit = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char default_brightness = 255;
constexpr int           cfg_count = 7;
constexpr auto          appname = "Gammy";
const std::string       settings_filename = "gammysettings.cfg";

constexpr std::array<const char*, cfg_count> cfg_str = {
    "minBrightness=",
    "maxBrightness=",
    "offset=",
    "temp=",
    "speed=",
    "threshold=",
    "updateRate=",
};

extern std::array<int, cfg_count> cfg;

enum settings {
    MinBr,
    MaxBr,
    Offset,
    Temp,
    Speed,
    Threshold,
    Polling_rate
};

extern int scrBr;
extern int polling_rate_min, polling_rate_max;

void setGDIBrightness(unsigned short brightness, int temp);

void checkInstance();
void checkGammaRange();
void readConfig();
void updateConfig();

#ifdef _WIN32
std::wstring getExecutablePath(bool);
void toggleRegkey(bool);

void getGDISnapshot(uint8_t* buf);
void setGDIBrightness(unsigned short brightness, int temp);
#else
std::string getHomePath(bool);
void sig_handler(int signo);
#endif

#endif // MAIN_H
