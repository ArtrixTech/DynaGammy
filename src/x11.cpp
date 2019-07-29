#include "x11.h"
#include "main.h"
#include <iostream>
#include <thread>
#include <X11/Xutil.h>
#include <cstring>
#include <X11/extensions/xf86vmode.h>

X11::X11()
{
    #ifdef dbg
    std::cout << "Initializing X11... ";
    #endif

    display = XOpenDisplay(nullptr);
    scr = DefaultScreenOfDisplay(display);
    w = unsigned(scr->width);
    h = unsigned(scr->height);
    bufLen = w * h * 4;

    /**Query XF86Vidmode extension */
    {
        int ev_base, err_base;
        if (!XF86VidModeQueryExtension(display, &ev_base, &err_base))
        {
            #ifdef dbg
            std::cout << "Failed to query XF86VidMode extension.\n";
            #endif
            return;
        }

        int major_ver, minor_ver;
        if (!XF86VidModeQueryVersion(display, &major_ver, &minor_ver))
        {
            std::cout << "Failed to query XF86VidMode version.\n";
            return;
        }

        #ifdef dbg
        std::cout << "XF86 Major ver: " << major_ver << " Minor ver: " << minor_ver << "\n";
        #endif
   }

    /**Get initial gamma ramp and size */
    {
        if (!XF86VidModeGetGammaRampSize(display, screen_num, &ramp_sz))
        {
            #ifdef dbg
            std::cout << "Failed to get XF86 gamma ramp size.\n";
            #endif
            return;
        }

        init_ramp = new uint16_t[3 * size_t(ramp_sz) * sizeof(uint16_t)];

        uint16_t* r = &init_ramp[0 * ramp_sz];
        uint16_t* g = &init_ramp[1 * ramp_sz];
        uint16_t* b = &init_ramp[2 * ramp_sz];

        if (!XF86VidModeGetGammaRamp(display, screen_num, ramp_sz, r, g, b))
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
    Window root;
    root = DefaultRootWindow(display);

    int x = 0, y = 0;
    XImage* img;

    img = XGetImage(display, root, x, y, w, h, AllPlanes, ZPixmap);
    std::this_thread::sleep_for(std::chrono::milliseconds(polling_rate_ms));
    memcpy(buf, img->data, bufLen);

    XDestroyImage(img);
}

void X11::fillRamp(uint16_t*& ramp, int amount, int temp)
{
    uint16_t* r = &ramp[0 * ramp_sz];
    uint16_t* g = &ramp[1 * ramp_sz];
    uint16_t* b = &ramp[2 * ramp_sz];

    double slope = 1. * 32 / 255;

    double gdiv = 1.;
    double bdiv = 1.;
    double tval = temp;

    double output = slope * amount;

    if(temp > 1)
    {
        bdiv += (tval / 100);
        gdiv += (tval / 270);
    }

    for (uint16_t i = 0; i < ramp_sz; i++)
    {
        double val = i * output;

        r[i] = uint16_t(val);
        g[i] = uint16_t(val / gdiv);
        b[i] = uint16_t(val / bdiv);
    }
}

void X11::setXF86Brightness(uint16_t scrBr, int temp)
{
    if (scrBr > default_brightness) {
        return;
    }

    uint16_t* ramp = new uint16_t[3 * size_t(ramp_sz) * sizeof(uint16_t)];

    fillRamp(ramp, scrBr, temp);

    delete[] ramp;

    XF86VidModeSetGammaRamp(display, 0, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);
}

void X11::setInitialGamma(bool set_previous)
{
#ifdef dbg
    std::cout << "Setting initial gamma...\n";
#endif

    if(set_previous)
    {
        //Sets the gamma to how it was before the program started
        XF86VidModeSetGammaRamp(display, screen_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
    }
    else
    {
        //Sets the gamma to default values
        uint16_t* ramp = new uint16_t[3 * ramp_sz * sizeof(uint16_t)];
        fillRamp(ramp, default_brightness, 1);
        XF86VidModeSetGammaRamp(display, screen_num, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);
        delete[] ramp;
    }
}

X11::~X11()
{
    XCloseDisplay(display);

    delete[] init_ramp;
}
