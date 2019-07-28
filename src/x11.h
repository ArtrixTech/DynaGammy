#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>
#undef None //Needed to avoid build error with qurl.h
#include <cstdint>

class X11
{
    Display* display;
    Screen* scr;

    int ramp_sz;
    int screen_num;
    uint16_t* init_ramp;

    size_t bufLen;

public:
    X11();
    unsigned w, h;

    void getX11Snapshot(uint8_t* buf);
    void fillRamp(uint16_t*& ramp, int amount, int temp);
    void setXF86Brightness(uint16_t scrBr, int temp);
    void setInitialGamma();

    ~X11();
};

extern X11 x11;

#endif // X11_H
