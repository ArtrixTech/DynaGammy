/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include "dspctl-xlib.h"
#include "defs.h"
#include "utils.h"

DspCtl::DspCtl()
{
	if (!XInitThreads()) {
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

	int ev_base, err_base;

	if (!XF86VidModeQueryExtension(dsp, &ev_base, &err_base)) {
		LOGW << "Failed to query XF86VidMode extension";
	}

	int major_ver, minor_ver;

	if (!XF86VidModeQueryVersion(dsp, &major_ver, &minor_ver)) {
		LOGW << "Failed to query XF86VidMode version";
	}

	LOGD << "XF86VidMode ver: " << major_ver << '.' << minor_ver;

	if (!XF86VidModeGetGammaRampSize(dsp, scr_num, &ramp_sz)) {
		LOGF << "Failed to get XF86 gamma ramp size";
		exit(EXIT_FAILURE);
	}

	if (ramp_sz == 0) {
		LOGF << "Invalid gamma ramp size";
		exit(EXIT_FAILURE);
	}

	init_ramp.resize(3 * size_t(ramp_sz) * sizeof(uint16_t));

	uint16_t *d = init_ramp.data(),
	         *r = &d[0 * ramp_sz],
	         *g = &d[1 * ramp_sz],
	         *b = &d[2 * ramp_sz];

	if (!XF86VidModeGetGammaRamp(dsp, scr_num, ramp_sz, r, g, b)) {
		LOGE << "Failed to get initial gamma ramp";
		initial_ramp_exists = false;
	}
}

void DspCtl::getSnapshot(std::vector<uint8_t> &buf) noexcept
{
	const auto img = XGetImage(dsp, root, 0, 0, w, h, AllPlanes, ZPixmap);
	buf.assign(img->data, img->data + (img->bytes_per_line * img->height));
	img->f.destroy_image(img);
}

/**
 * The ramp multiplier equals 32 when ramp_sz = 2048, 64 when 1024, etc.
 * Assuming ramp_sz = 2048 and pure state (default brightness/temp)
 * the RGB channels look like:
 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
 */
void DspCtl::fillRamp(std::vector<uint16_t> &ramp, const int brt_step, const int temp_step)
{
	auto r = &ramp[0 * ramp_sz],
	     g = &ramp[1 * ramp_sz],
	     b = &ramp[2 * ramp_sz];

	const double r_mult = interpTemp(temp_step, 0),
	             g_mult = interpTemp(temp_step, 1),
	             b_mult = interpTemp(temp_step, 2);

	const int    ramp_mult = (UINT16_MAX + 1) / ramp_sz;
	const double brt_mult  = normalize(0, brt_slider_steps, brt_step) * ramp_mult;

	for (int i = 0; i < ramp_sz; ++i) {
		const int val = std::clamp(int(i * brt_mult), 0, UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}
}

void DspCtl::setGamma(int scr_br, int temp)
{
	std::vector<uint16_t> r (3 * ramp_sz * sizeof(uint16_t));
	fillRamp(r, scr_br, temp);
	XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &r[0*ramp_sz], &r[1*ramp_sz], &r[2*ramp_sz]);
}

void DspCtl::setInitialGamma(bool set_previous)
{
	if (set_previous && initial_ramp_exists) {
		LOGI << "Setting previous gamma";
		XF86VidModeSetGammaRamp(dsp, scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
	} else {
		LOGI << "Setting pure gamma";
		setGamma(brt_slider_steps, 0);
	}
}

DspCtl::~DspCtl()
{
	if (dsp)
		XCloseDisplay(dsp);
}
