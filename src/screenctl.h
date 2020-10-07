#ifndef SCREENCTL_H
#define SCREENCTL_H

#ifdef _WIN32
#include "dxgidupl.h"
#else
#include "x11.h"
#endif

class ScreenCtl
{
public:
	ScreenCtl();
	void setGamma(int, int);

	uint64_t getResolution();

	bool initDXGI();
	void getSnapshot(std::vector<uint8_t> &buf) noexcept;
	void setInitialGamma(bool);

private:
#ifdef _WIN32
	DXGIDupl dx;
	bool useDXGI;
#else
	X11 x11;
#endif

};

#endif // SCREENCTL_H
