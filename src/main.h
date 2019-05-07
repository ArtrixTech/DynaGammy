#ifndef MAIN_H
#define MAIN_H

#define dbg

#include <string>

constexpr unsigned char min_brightness_limit = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char default_brightness = 255;
constexpr unsigned char settings_count = 7;
constexpr auto          settings_filename = L"gammySettings.cfg";

extern unsigned char	min_brightness;
extern unsigned char	max_brightness;
extern unsigned short	offset;
extern unsigned char	speed;
extern unsigned char	temp;
extern unsigned char	threshold;
extern unsigned short	polling_rate_ms;
extern unsigned short	polling_rate_min, polling_rate_max;

extern unsigned short scrBr;

void setGDIBrightness(unsigned short brightness, int temp);

void checkInstance();
void checkGammaRange();
void readSettings();

std::wstring getExecutablePath(bool);

#endif // MAIN_H
