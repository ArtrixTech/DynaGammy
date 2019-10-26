/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef _WIN32
#include "x11.h"
#include <iostream>
#include <cstring>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include "utils.h"
#include "math.h"

X11::X11()
{
#ifdef dbg
    std::cout << "Initializing XDisplay ";
#endif

    XInitThreads();

    dsp = XOpenDisplay(nullptr);

    scr = DefaultScreenOfDisplay(dsp);
    scr_num = XDefaultScreen(dsp);

#ifdef dbg
    std::cout << "(scr " << scr_num << ")\n";
#endif

    root = DefaultRootWindow(dsp);
    w = unsigned(scr->width);
    h = unsigned(scr->height);

    /**Query XF86Vidmode extension */
    {
        int ev_base, err_base;

        if (!XF86VidModeQueryExtension(dsp, &ev_base, &err_base))
        {
#ifdef dbg
            std::cout << "Failed to query XF86VidMode extension.\n";
#endif
            return;
        }

        int major_ver, minor_ver;
        if (!XF86VidModeQueryVersion(dsp, &major_ver, &minor_ver))
        {
            std::cout << "Failed to query XF86VidMode version.\n";
            return;
        }

#ifdef dbg
        std::cout << "XF86VidMode ver: " << major_ver << "." << minor_ver << "\n";
#endif
   }

    /**Get initial gamma ramp and size */
    {
        if (!XF86VidModeGetGammaRampSize(dsp, scr_num, &ramp_sz))
        {
#ifdef dbg
            std::cout << "Failed to get XF86 gamma ramp size.\n";
#endif
            return;
        }

        if(ramp_sz == 0)
        {
#ifdef dbg
            std::cout << "Invalid gamma ramp size.\n";
#endif
            exit(EXIT_FAILURE);
        }

        init_ramp.resize(3 * size_t(ramp_sz) * sizeof(uint16_t));

        uint16_t *d = init_ramp.data(),
                 *r = &d[0 * ramp_sz],
                 *g = &d[1 * ramp_sz],
                 *b = &d[2 * ramp_sz];

        if (!XF86VidModeGetGammaRamp(dsp, scr_num, ramp_sz, r, g, b))
        {
#ifdef dbg
            std::cout << "Failed to get initial gamma ramp.\n";
#endif
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

void X11::fillRamp(std::vector<uint16_t> &ramp, int brightness, int temp)
{
    auto r = &ramp[0 * ramp_sz],
         g = &ramp[1 * ramp_sz],
         b = &ramp[2 * ramp_sz];

    std::array<double, 3> c{1.0, 1.0, 1.0};

    setColors(temp, c);

    /*
     * This equals 32 when ramp_sz = 2048, 64 when 1024, etc.
     * Assuming ramp_sz = 2048 and pure state (default brightness/temp)
     * each color channel looks like:
     * {0, 32, 64, 96, ... UINT16_MAX - 32} */
    const int pure_ramp_steps = (UINT16_MAX + 1) / ramp_sz;

    for (int32_t i = 0; i < ramp_sz; ++i)
    {
        double val = i * pure_ramp_steps * (brightness / 255.);

        if(val > UINT16_MAX) val = UINT16_MAX;

        r[i] = uint16_t(val * c[0]);
        g[i] = uint16_t(val * c[1]);
        b[i] = uint16_t(val * c[2]);
    }
}

void X11::setXF86Gamma(int scr_br, int temp)
{
    std::vector<uint16_t> r (3 * size_t(ramp_sz) * sizeof(uint16_t));

    fillRamp(r, scr_br, temp);

    XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &r[0*ramp_sz], &r[1*ramp_sz], &r[2*ramp_sz]);
}

void X11::setInitialGamma(bool set_previous)
{
    Display *d = XOpenDisplay(nullptr);

    if(set_previous && initial_ramp_exists)
    {
#ifdef dbg
            std::cout << "Setting previous gamma\n";
#endif

        XF86VidModeSetGammaRamp(d, scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
    }
    else
    {
#ifdef dbg
            std::cout << "Setting pure gamma\n";
#endif
        X11::setXF86Gamma(255, 0);
    }
  
    XCloseDisplay(d);
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

#endif
