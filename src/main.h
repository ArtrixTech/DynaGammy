#ifndef MAIN_H
#define MAIN_H

#define dbg

#include <string>
#include <array>

constexpr unsigned char min_brightness_limit = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char default_brightness = 255;
constexpr int           cfg_count = 7;
constexpr auto          appname = "Gammy";
const std::string       settings_filename = "gammysettings.cfg";

constexpr auto min_br_str       = "minBrightness=";
constexpr auto max_br_str       = "maxBrightness=";
constexpr auto offset_str       = "offset=";
constexpr auto speed_str        = "speed=";
constexpr auto temp_str         = "temp=";
constexpr auto threshold_str    = "threshold=";
constexpr auto polling_rate_str = "updateRate=";

extern int polling_rate_min, polling_rate_max;

extern std::array<std::pair<std::string, int>, cfg_count> cfg;

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
