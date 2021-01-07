/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/XShm.h>
#include "dspctl-xlib.h"
#include "defs.h"
#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>

XLib::XLib()
{
	if (!XInitThreads()) {
		LOGE << "Failed to initialize XThreads. App may crash unexpectedly.";
	}

	dsp      = XOpenDisplay(nullptr);
	root_wnd = DefaultRootWindow(dsp);
	scr      = DefaultScreenOfDisplay(dsp);
	scr_num  = XDefaultScreen(dsp);

	LOGD << "XDisplay initialized on screen " << scr_num;
}

XLib::~XLib()
{
	if (dsp)
		XCloseDisplay(dsp);
}

int XLib::getScreenBrightness() noexcept
{
	const auto img = XGetImage(dsp, root_wnd, 0, 0, scr->width, scr->height, AllPlanes, ZPixmap);
	int brt = calcBrightness(reinterpret_cast<uint8_t*>(img->data), img->bytes_per_line * img->height, img->bits_per_pixel / 8, 1024);
	img->f.destroy_image(img);
	return brt;
}

// Vidmode ---------------------------------------------------------------

Vidmode::Vidmode()
{
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

Vidmode::~Vidmode()
{
	init_ramp.clear();
}

void Vidmode::fillRamp(std::vector<uint16_t> &ramp, const int brt_step, const int temp_step)
{
	/**
	 * The ramp multiplier equals 32 when ramp_sz = 2048, 64 when 1024, etc.
	 * Assuming ramp_sz = 2048 and pure state (default brightness/temp)
	 * the RGB channels look like:
	 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
	 */
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

void Vidmode::setGamma(int scr_br, int temp)
{
	std::vector<uint16_t> ramp (3 * ramp_sz * sizeof(uint16_t));
	fillRamp(ramp, scr_br, temp);
	XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &ramp[0], &ramp[ramp_sz], &ramp[2 * ramp_sz]);
}

void Vidmode::setInitialGamma(bool set_previous)
{
	if (set_previous && initial_ramp_exists) {
		LOGI << "Setting previous gamma";
		XF86VidModeSetGammaRamp(dsp, scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
	} else {
		LOGI << "Setting pure gamma";
		setGamma(brt_slider_steps, 0);
	}
}

// XShm ------------------------------------------------------------------

Xshm::Xshm()
{
	int ignore;

	if (!XQueryExtension(dsp, "MIT-SHM", &ignore, &ignore, &ignore)) {
		LOGE << "Failed to query XShm";
	}

	int major, minor;
	int pixmaps;

	if (!XShmQueryVersion(dsp, &major, &minor, &pixmaps)) {
		LOGE << "Failed to query Xshm version";
	}

	LOGV << "XImage support: " << (pixmaps == 1);
	LOGV << "Pixmap support: " << (pixmaps == 2);

	vis = XDefaultVisual(dsp, 0);
	shi = allocXImage();

	if (!shi) {
		LOGE << "Shared image unavailable";
	}
}

Xshm::~Xshm()
{
	XDestroyImage(shi);
}

XImage* Xshm::allocXImage()
{
	XImage *img;
	img = XShmCreateImage(dsp, vis, scr->root_depth, ZPixmap, nullptr, &shminfo, scr->width, scr->height);

	if (!img) {
		LOGE << "XShmCreateImage failed";
		return nullptr;
	}

	shminfo.shmid    = shmget(IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT|0777);
	shminfo.shmaddr  = img->data = reinterpret_cast<char*>(shmat(shminfo.shmid, 0, 0));
	shminfo.readOnly = False;

	if (!XShmAttach(dsp, &shminfo)) {
		XFlush(dsp);
		img->f.destroy_image(img);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
		return nullptr;
	}

	XSync(dsp, False);
	shmctl(shminfo.shmid, IPC_RMID, 0);

	return img;
}

int Xshm::getScreenBrightness() noexcept
{
	XShmGetImage(dsp, root_wnd, shi, 0, 0, AllPlanes);
	int brt = calcBrightness(reinterpret_cast<uint8_t*>(shi->data), shi->bytes_per_line * shi->height, shi->bits_per_pixel / 8, 1024);
	return brt;
}

