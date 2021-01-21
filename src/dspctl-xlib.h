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
	Window  root_wnd;
	Screen  *scr;
	int     scr_num;
};

class Vidmode : public XLib
{
public:
	Vidmode();
	~Vidmode();
	void setGamma(int, int);
	void setInitialGamma(bool);
protected:
	int ramp_sz;
	bool initial_ramp_exists = true;
	std::vector<uint16_t> init_ramp;

	void fillRamp(std::vector<uint16_t> &ramp, const int brightness, const int temp);
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
	Visual *vis;
	XImage* createImage();
};

typedef Xshm DspCtl;

#endif // X11_H
