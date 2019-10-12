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

void X11::getX11Snapshot(uint8_t* buf)
{
    //XLockDisplay(dsp);
    XImage* img = XGetImage(dsp, root, 0, 0, w, h, AllPlanes, ZPixmap);
    //XUnlockDisplay(dsp);

    size_t len = size_t(img->bytes_per_line * img->height);
    memcpy(buf, img->data, len);
    XDestroyImage(img);
}

double interpolate_color(double c1, double c2)
{
    std::cout << "Colors to avg: " << c1 << " | " << c2 << '\n';

    double avg = (c1 + c2) / 2;

    std::cout << "Average: " << avg << '\n';

    return avg;
}

void X11::fillRamp(uint16_t *ramp, int brightness, int temp)
{
    static constexpr size_t sz = 46;
    static constexpr std::array<double, sz * 3> ingo_thies_table =
    {
        1.00000000,  0.54360078,  0.08679949, /* 2000K */
        1.00000000,  0.56618736,  0.14065513,
        1.00000000,  0.58734976,  0.18362641,
        1.00000000,  0.60724493,  0.22137978,
        1.00000000,  0.62600248,  0.25591950,
        1.00000000,  0.64373109,  0.28819679,
        1.00000000,  0.66052319,  0.31873863,
        1.00000000,  0.67645822,  0.34786758,
        1.00000000,  0.69160518,  0.37579588,
        1.00000000,  0.70602449,  0.40267128,
        1.00000000,  0.71976951,  0.42860152,
        1.00000000,  0.73288760,  0.45366838,
        1.00000000,  0.74542112,  0.47793608,
        1.00000000,  0.75740814,  0.50145662,
        1.00000000,  0.76888303,  0.52427322,
        1.00000000,  0.77987699,  0.54642268,
        1.00000000,  0.79041843,  0.56793692,
        1.00000000,  0.80053332,  0.58884417,
        1.00000000,  0.81024551,  0.60916971,
        1.00000000,  0.81957693,  0.62893653,
        1.00000000,  0.82854786,  0.64816570,
        1.00000000,  0.83717703,  0.66687674,
        1.00000000,  0.84548188,  0.68508786,
        1.00000000,  0.85347859,  0.70281616,
        1.00000000,  0.86118227,  0.72007777,
        1.00000000,  0.86860704,  0.73688797,
        1.00000000,  0.87576611,  0.75326132,
        1.00000000,  0.88267187,  0.76921169,
        1.00000000,  0.88933596,  0.78475236,
        1.00000000,  0.89576933,  0.79989606,
        1.00000000,  0.90198230,  0.81465502,
        1.00000000,  0.90963069,  0.82838210,
        1.00000000,  0.91710889,  0.84190889,
        1.00000000,  0.92441842,  0.85523742,
        1.00000000,  0.93156127,  0.86836903,
        1.00000000,  0.93853986,  0.88130458,
        1.00000000,  0.94535695,  0.89404470,
        1.00000000,  0.95201559,  0.90658983,
        1.00000000,  0.95851906,  0.91894041,
        1.00000000,  0.96487079,  0.93109690,
        1.00000000,  0.97107439,  0.94305985,
        1.00000000,  0.97713351,  0.95482993,
        1.00000000,  0.98305189,  0.96640795,
        1.00000000,  0.98883326,  0.97779486,
        1.00000000,  0.99448139,  0.98899179,
        1.00000000,  1.00000000,  1.00000000 /* 6500K */
    };

    uint16_t *r = &ramp[0 * ramp_sz],
             *g = &ramp[1 * ramp_sz],
             *b = &ramp[2 * ramp_sz];

    double green {1.00000000};
    double blue  {1.00000000};

    if(temp > 0)
    {
        size_t tick = size_t(temp / 3);

        if(tick > sz) tick = sz;
        else if(tick < 1) tick = 1;

        std::cout << "Temp_step: " << temp << '\n';
        std::cout << "Tick: " << tick << '\n';

        size_t gpos = (sz - tick) * 3 + 1;
        size_t bpos = (sz - tick) * 3 + 2;

        size_t next_gpos = gpos + 3;
        if(next_gpos > temp_steps) next_gpos = gpos;

        size_t next_bpos = bpos + 3;
        if(next_bpos > temp_steps) next_bpos = bpos;

        //green = interpolate_color(ingo_thies_table.at(gpos), ingo_thies_table.at(next_gpos));
        //blue  = interpolate_color(ingo_thies_table.at(bpos), ingo_thies_table.at(next_bpos));

        green = ingo_thies_table.at(gpos);
        blue  = ingo_thies_table.at(bpos);

        std::cout << green << '\n';
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
       std::cout << "setXF86Brightness failed.\n";
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
