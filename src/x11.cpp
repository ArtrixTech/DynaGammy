/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "x11.h"
#include <iostream>
#include <cstring>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include "utils.h"
#include "defs.h"
#include <algorithm>
#include <cmath>

X11::X11()
{
	if(!XInitThreads())
	{
		LOGE << "Failed to initialize XThreads. App may crash unexpectedly.";
	}

	LOGD << "Initializing XDisplay...";

	dsp     = XOpenDisplay(nullptr);
	root    = DefaultRootWindow(dsp);

	scr     = DefaultScreenOfDisplay(dsp);
	scr_num = XDefaultScreen(dsp);

	LOGD << "XDisplay initialized on screen " << scr_num;

	w = uint32_t(scr->width);
	h = uint32_t(scr->height);

	// Query XF86Vidmode extension
	{
		int ev_base, err_base;

		if (!XF86VidModeQueryExtension(dsp, &ev_base, &err_base))
		{
			LOGW << "Failed to query XF86VidMode extension";
		}

		int major_ver, minor_ver;

		if (!XF86VidModeQueryVersion(dsp, &major_ver, &minor_ver))
		{
			LOGW << "Failed to query XF86VidMode version";
		}

		LOGD << "XF86VidMode ver: " << major_ver << '.' << minor_ver;
	}

	// Get initial gamma ramp and size
	{
		if (!XF86VidModeGetGammaRampSize(dsp, scr_num, &ramp_sz))
		{
			LOGF << "Failed to get XF86 gamma ramp size";
			exit(EXIT_FAILURE);
		}

		if(ramp_sz == 0)
		{
			LOGF << "Invalid gamma ramp size";
			exit(EXIT_FAILURE);
		}

		init_ramp.resize(3 * size_t(ramp_sz) * sizeof(uint16_t));

		uint16_t *d = init_ramp.data(),
			 *r = &d[0 * ramp_sz],
			 *g = &d[1 * ramp_sz],
			 *b = &d[2 * ramp_sz];

		if (!XF86VidModeGetGammaRamp(dsp, scr_num, ramp_sz, r, g, b))
		{
			LOGE << "Failed to get initial gamma ramp";
			initial_ramp_exists = false;
		}
	}
}

void X11::getX11Snapshot(std::vector<uint8_t> &buf) noexcept
{
	const auto img = XGetImage(dsp, root, 0, 0, w, h, AllPlanes, ZPixmap);

	memcpy(buf.data(), reinterpret_cast<uint8_t*>(img->data), buf.size());

	img->f.destroy_image(img);
}

void X11::fillRamp(std::vector<uint16_t> &ramp, const int brightness, const int temp_step)
{
	auto r = &ramp[0 * ramp_sz],
	     g = &ramp[1 * ramp_sz],
	     b = &ramp[2 * ramp_sz];

	std::array<double, 3> c{1.0, 1.0, 1.0};

	setColors(temp_step, c);

	/* This equals 32 when ramp_sz = 2048, 64 when 1024, etc.
	*  Assuming ramp_sz = 2048 and pure state (default brightness/temp)
	*  each color channel looks like:
	* { 0, 32, 64, 96, ... UINT16_MAX - 32 } */
	const int ramp_mult = (UINT16_MAX + 1) / ramp_sz;

	for (int32_t i = 0; i < ramp_sz; ++i)
	{
		const int val = std::clamp(int(normalize(0, brt_slider_steps, brightness) * ramp_mult * i), 0, UINT16_MAX);

		r[i] = uint16_t(val * c[0]);
		g[i] = uint16_t(val * c[1]);
		b[i] = uint16_t(val * c[2]);
	}
}

void X11::setXF86Gamma(int scr_br, int temp)
{
	std::vector<uint16_t> r (3 * ramp_sz * sizeof(uint16_t));

	fillRamp(r, scr_br, temp);

	XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &r[0*ramp_sz], &r[1*ramp_sz], &r[2*ramp_sz]);
}

void X11::setInitialGamma(bool set_previous)
{
	if(set_previous && initial_ramp_exists)
	{
		LOGI << "Setting previous gamma";
		XF86VidModeSetGammaRamp(dsp, scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
	}
	else
	{
		LOGI << "Setting pure gamma";
		X11::setXF86Gamma(brt_slider_steps, 0);
	}
}

uint32_t X11::getWidth()
{
	return w;
}

uint32_t X11::getHeight()
{
	return h;
}

X11::~X11()
{
	if(dsp) XCloseDisplay(dsp);
}
