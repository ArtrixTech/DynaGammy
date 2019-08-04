#ifdef __linux
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
    scr_num = DefaultScreen(display);
    root = DefaultRootWindow(display);
    w = unsigned(scr->width);
    h = unsigned(scr->height);

    /**Query XF86Vidmode extension */
    {
        XInitExtension(display, "XF86VidMode");

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
        if (!XF86VidModeGetGammaRampSize(display, scr_num, &ramp_sz))
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

        if (!XF86VidModeGetGammaRamp(display, scr_num, ramp_sz, r, g, b))
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
    XImage* img = XGetImage(display, root, 0, 0, w, h, AllPlanes, ZPixmap);
    size_t len = size_t(img->bytes_per_line * img->height);
    memcpy(buf, img->data, len);
    XDestroyImage(img);

    std::this_thread::sleep_for(std::chrono::milliseconds(polling_rate_ms));
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

    std::cout << output << '\n';

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

    bool r = XF86VidModeSetGammaRamp(display, 0, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);

    delete[] ramp;

    if(!r)
    {
       #ifdef dbg
       std::cout << "Failed to set XF86 Gamma ramp.\n";
       #endif

       delete[] ramp;

       return;
    }

}

void X11::setInitialGamma(bool set_previous)
{
#ifdef dbg
    std::cout << "Setting initial gamma...\n";
#endif

    if(set_previous)
    {
        //Sets the gamma to how it was before the program started
        XF86VidModeSetGammaRamp(display, scr_num, ramp_sz, &init_ramp[0*ramp_sz], &init_ramp[1*ramp_sz], &init_ramp[2*ramp_sz]);
    }
    else
    {
        //Sets the gamma to default values
        uint16_t* ramp = new uint16_t[3 * ramp_sz * sizeof(uint16_t)];
        fillRamp(ramp, default_brightness, 1);
        XF86VidModeSetGammaRamp(display, scr_num, ramp_sz, &ramp[0*ramp_sz], &ramp[1*ramp_sz], &ramp[2*ramp_sz]);
        delete[] ramp;
    }
}

X11::~X11()
{
    XCloseDisplay(display);

    delete[] init_ramp;
}

#endif
