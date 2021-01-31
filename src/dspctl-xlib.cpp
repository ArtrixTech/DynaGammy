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

	dsp              = XOpenDisplay(nullptr);
	default_root_wnd = DefaultRootWindow(dsp);
	default_scr      = DefaultScreenOfDisplay(dsp);
	default_scr_num  = XDefaultScreen(dsp);
	scr_count        = XScreenCount(dsp);
	LOGV << "XDisplay initialized. Screens: " << scr_count;
}

XLib::~XLib()
{
	if (dsp)
		XCloseDisplay(dsp);
}

int XLib::getScreenBrightness() noexcept
{
	const auto img = XGetImage(dsp, default_root_wnd, 0, 0, default_scr->width, default_scr->height, AllPlanes, ZPixmap);
	int brt = calcBrightness(reinterpret_cast<uint8_t*>(img->data), img->bytes_per_line * img->height, img->bits_per_pixel / 8, 1024);
	img->f.destroy_image(img);
	return brt;
}

// Vidmode ---------------------------------------------------------------

Vidmode::Vidmode()
{
	int ev_base, err_base;

	if (!XF86VidModeQueryExtension(dsp, &ev_base, &err_base)) {
		LOGE << "Failed to query VidMode";
	}

	if (!XF86VidModeGetGammaRampSize(dsp, default_scr_num, &ramp_sz)) {
		LOGF << "Failed to get gamma ramp size";
		exit(EXIT_FAILURE);
	}

	if (ramp_sz == 0) {
		LOGF << "Invalid gamma ramp size";
		exit(EXIT_FAILURE);
	}

	ramp.resize(3 * ramp_sz * sizeof(uint16_t));
	init_ramp.resize(3 * ramp_sz * sizeof(uint16_t));

	uint16_t *d = init_ramp.data(),
	         *r = &d[0 * ramp_sz],
	         *g = &d[1 * ramp_sz],
	         *b = &d[2 * ramp_sz];

	if (!XF86VidModeGetGammaRamp(dsp, default_scr_num, ramp_sz, r, g, b)) {
		LOGE << "Failed to get initial gamma ramp";
		initial_ramp_exists = false;
	}
}

Vidmode::~Vidmode()
{
	init_ramp.clear();
}

void Vidmode::fillRamp(const int brt_step, const int temp_step)
{
	/**
	 * The ramp multiplier equals 32 when ramp_sz = 2048, 64 when 1024, etc.
	 * Assuming ramp_sz = 2048 and pure state (default brightness/temp)
	 * the RGB channels look like:
	 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
	 */
	uint16_t *r = &ramp[0 * ramp_sz];
	uint16_t *g = &ramp[1 * ramp_sz];
	uint16_t *b = &ramp[2 * ramp_sz];

	const double r_mult = interpTemp(temp_step, 0),
	             g_mult = interpTemp(temp_step, 1),
	             b_mult = interpTemp(temp_step, 2);

	const int    ramp_mult = (UINT16_MAX + 1) / ramp_sz;
	const double brt_mult  = normalize(brt_step, 0, brt_steps_max) * ramp_mult;

	for (int i = 0; i < ramp_sz; ++i) {
		const int val = std::clamp(int(i * brt_mult), 0, UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}
}

void Vidmode::setGamma(int scr_br, int temp)
{
	fillRamp(scr_br, temp);
	XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &ramp[0], &ramp[ramp_sz], &ramp[2 * ramp_sz]);
}

void Vidmode::setInitialGamma(bool set_previous)
{
	if (set_previous && initial_ramp_exists) {
		LOGI << "Setting previous gamma";
		XF86VidModeSetGammaRamp(dsp, default_scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
	} else {
		LOGI << "Setting pure gamma";
		setGamma(brt_steps_max, 0);
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

	default_vis = XDefaultVisual(dsp, 0);
	shi = createImage();

	if (!shi) {
		LOGE << "Shared image unavailable";
	}
}

Xshm::~Xshm()
{
	XDestroyImage(shi);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid, IPC_RMID, nullptr);
}

XImage* Xshm::createImage()
{
	XImage *img = XShmCreateImage(dsp, default_vis, default_scr->root_depth, ZPixmap, nullptr, &shminfo, default_scr->width, default_scr->height);

	if (!img) {
		LOGF << "XShmCreateImage failed";
		exit(1);
	}

	shminfo.shmid = shmget(IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT | 0600);

	if (shminfo.shmid == -1) {
		LOGF << "shmget failed";
		exit(1);
	}

	void *shm = shmat(shminfo.shmid, nullptr, SHM_RDONLY);

	if (shm == reinterpret_cast<void*>(-1)) {
		LOGF << "shmat failed";
		exit(1);
	}

	shminfo.shmaddr = img->data = reinterpret_cast<char*>(shm);
	shminfo.readOnly = False;
	int status = XShmAttach(dsp, &shminfo);

	if (!status) {
		LOGF << "XShmAttach failed with code: " << status;
		exit(1);
	}

	return img;
}

int Xshm::getScreenBrightness() noexcept
{
	XShmGetImage(dsp, default_root_wnd, shi, 0, 0, AllPlanes);
	return calcBrightness(reinterpret_cast<uint8_t*>(shi->data), shi->bytes_per_line * shi->height, shi->bits_per_pixel / 8, 1024);
}

