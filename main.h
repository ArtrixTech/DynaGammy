#ifndef MAIN_H
#define MAIN_H

//#define dbg

constexpr unsigned char min_brightness_limit = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char default_brightness = 255;

constexpr float gdivs[] = { 1.f, 1.1f, 1.2f, 1.4f };
constexpr float bdivs[] = { 1.f, 1.35f, 1.7f, 2.4f };

constexpr unsigned char settings_count = 7;

extern unsigned char	min_brightness;
extern unsigned char	max_brightness;
extern unsigned short	offset;
extern unsigned char	speed;
extern unsigned char	temp;
extern unsigned char	threshold;
extern unsigned short	polling_rate_ms;
extern unsigned short	polling_rate_min, polling_rate_max;

extern unsigned short scrBr;
extern unsigned short targetScrBr;

void setGDIBrightness(unsigned short brightness, float gdiv, float bdiv);

void checkInstance();
void checkGammaRange();
void readSettings();

#endif // MAIN_H
