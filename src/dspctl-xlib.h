/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <cstdint>
#include <vector>

class XLib
{
public:
	XLib();
	~XLib();
	int getScreenBrightness() noexcept;
protected:
	Display *dsp;
	int scr_count;
	Window  default_root_wnd;
	Screen  *default_scr;
	int     default_scr_num;
};

class Vidmode : public XLib
{
public:
	Vidmode();
	~Vidmode();
	void setGamma(int, int);
	void setInitialGamma(bool);
private:
	int ramp_sz;
	bool initial_ramp_exists = true;
	std::vector<uint16_t> ramp;
	std::vector<uint16_t> init_ramp;
	void fillRamp(const int brightness, const int temp);
};

class Xshm : public Vidmode
{
public:
	Xshm();
	~Xshm();
	int getScreenBrightness() noexcept;
private:
	XShmSegmentInfo shminfo;
	XImage *shi;
	Visual *default_vis;
	XImage* createImage();
};

typedef Xshm DspCtl;

#endif // X11_H
