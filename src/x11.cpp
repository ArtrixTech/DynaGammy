/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef _WIN32
#include "x11.h"
#include "main.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include "utils.h"

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

        init_ramp.resize(3 * size_t(ramp_sz) * sizeof(uint16_t));

        uint16_t *d = init_ramp.data(),
                 *r = &d[0 * ramp_sz],
                 *g = &d[1 * ramp_sz],
                 *b = &d[2 * ramp_sz];

        if (!XF86VidModeGetGammaRamp(dsp, scr_num, ramp_sz, r, g, b))
        {
            #ifdef dbg
            std::cout << "Failed to get XF86 gamma ramp.\n";
            #endif
            return;
        }
    }
}

unsigned X11::getWidth() {
    return w;
}

unsigned X11::getHeight() {
    return h;
}

void X11::getX11Snapshot(uint8_t *buf)
{
    XImage *img = XGetImage(dsp, root, 0, 0, w, h, AllPlanes, ZPixmap);
    size_t len = size_t(img->bytes_per_line * img->height);

    memcpy(buf, img->data, len);

    XDestroyImage(img);
}

void X11::fillRamp(uint16_t *ramp, int brightness, int temp)
{
    auto *r = &ramp[0 * ramp_sz],
         *g = &ramp[1 * ramp_sz],
         *b = &ramp[2 * ramp_sz];

    double green {1.0};
    double blue  {1.0};

    size_t tick = size_t(temp / temp_mult);

    if(temp > 0)
    {
        if(tick > temp_arr_entries) tick = temp_arr_entries;
        else if(tick < 1) tick = 1;

        size_t gpos = (temp_arr_entries - tick) * 3 + 1;
        size_t bpos = (temp_arr_entries - tick) * 3 + 2;

        green = ingo_thies_table.at(gpos);
        blue  = ingo_thies_table.at(bpos);
    }

    auto br = convertToRange(double(brightness), 0, 255, 0, 32);

    for (int32_t i = 0; i < ramp_sz; ++i)
    {
        int32_t val = int32_t(i * br);

        if(val > UINT16_MAX) val = UINT16_MAX;

        r[i] = uint16_t(val);
        g[i] = uint16_t(val * green);
        b[i] = uint16_t(val * blue);
    }
}

void X11::setXF86Gamma(int scr_br, int temp)
{
    std::vector<uint16_t> ramp_v(3 * size_t(ramp_sz) * sizeof(uint16_t));

    uint16_t *ramp = ramp_v.data();
    fillRamp(ramp, scr_br, temp);

    bool r = XF86VidModeSetGammaRamp(dsp, 0, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);

#ifdef dbg
    if(!r)
    {
       std::cout << "XF86VidModeSetGammaRamp failed.\n";
    }
#endif
}

void X11::setInitialGamma(bool set_previous)
{
    Display* d = XOpenDisplay(nullptr);

    if(set_previous)
    {
        #ifdef dbg
            std::cout << "Setting previous gamma\n";
        #endif

        size_t r = size_t(ramp_sz);
        XF86VidModeSetGammaRamp(d, scr_num, ramp_sz, &init_ramp[0], &init_ramp[r], &init_ramp[2 * r]);
    }
    else
    {
        #ifdef dbg
            std::cout << "Setting pure gamma\n";
        #endif
        X11::setXF86Gamma(default_brightness, 1);
    }
  
    XCloseDisplay(d);
}

X11::~X11()
{
    if(dsp) XCloseDisplay(dsp);
}

#endif
