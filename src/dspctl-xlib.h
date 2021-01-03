/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>
#include <cstdint>
#include <vector>

/**
 * @brief Xlib display controller
 */
class DspCtl
{
public:
	DspCtl();
	~DspCtl();

	int  getScreenBrightness() noexcept;
	void setGamma(int brt, int temp);
	void setInitialGamma(bool set_previous);
private:
	Display *dsp;
	Screen *scr;
	Window root;

	int ramp_sz;
	int scr_num;

	std::vector<uint16_t> init_ramp;
	bool initial_ramp_exists = true;

	unsigned w, h;

	void fillRamp(std::vector<uint16_t> &ramp, const int brightness, const int temp);
};

#endif // X11_H
