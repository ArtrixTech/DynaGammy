#ifndef SCREENCTL_H
#define SCREENCTL_H

class X11;
class DXGIDupl;

#include <vector>

#ifdef _WIN32
#include "dxgidupl.h"
#else
#include "x11.h"
#undef Status
#undef Bool
#undef CursorShape
#undef None
#endif

class ScreenCtl
{
public:
	ScreenCtl();
	uint64_t getResolution();
	void setGamma(int, int);
	void getSnapshot(std::vector<uint8_t> &buf) noexcept;
	void setInitialGamma(bool);
protected:
	bool initDXGI();
private:
#ifdef _WIN32
	DXGIDupl dx;
	bool useDXGI;
#else
	X11 x11;
#endif

};

#endif // SCREENCTL_H
