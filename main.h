#ifndef MAIN_H
#define MAIN_H

//#define dbg

constexpr unsigned char MIN_BRIGHTNESS_LIMIT = 64; //Below 127, SetDeviceGammaRamp doesn't work without the registry edit
constexpr unsigned char DEFAULT_BRIGHTNESS = 255;

constexpr unsigned char settingsCount = 7;

constexpr float gdivs[] = { 1.f, 1.1f, 1.2f, 1.4f };
constexpr float bdivs[] = { 1.f, 1.35f, 1.7f, 2.4f };

extern unsigned char	MIN_BRIGHTNESS;
extern unsigned char	MAX_BRIGHTNESS;
extern unsigned short	OFFSET;
extern unsigned char	SPEED;
extern unsigned char	TEMP;
extern unsigned char	THRESHOLD;
extern unsigned short	UPDATE_TIME_MS;
extern unsigned short	UPDATE_TIME_MIN, UPDATE_TIME_MAX;

extern unsigned short scrBr;
extern unsigned short targetScrBr;

void setGDIBrightness(unsigned short brightness, float gdiv, float bdiv);

void checkInstance();
void checkGammaRange();
void readSettings();

#endif // MAIN_H
