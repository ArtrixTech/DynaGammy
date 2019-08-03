#ifndef MAIN_H
#define MAIN_H

#define dbg

#include <string>

constexpr unsigned char min_brightness_limit = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char default_brightness = 255;
constexpr int           settings_count = 7;
constexpr auto          appname = "Gammy";
const std::string       settings_filename = "gammysettings.cfg";

extern int min_brightness;
extern int max_brightness;
extern int offset;
extern int speed;
extern int temp;
extern int threshold;
extern int polling_rate_ms;
extern int polling_rate_min, polling_rate_max;

extern int scrBr;

void setGDIBrightness(unsigned short brightness, int temp);

void checkInstance();
void checkGammaRange();
void readSettings();

#ifdef _WIN32
std::wstring getExecutablePath(bool);
#else
std::string getAppPath(bool);
#endif



#endif // MAIN_H
