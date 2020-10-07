#include "screenctl.h"
#include "utils.h"
#include "cfg.h"
#include <chrono>
#include <thread>

ScreenCtl::ScreenCtl() {}

bool ScreenCtl::initDXGI() {

#ifdef _WIN32
	useDXGI = dx.initDXGI();

	if (!useDXGI) {
		return false;
	}
#endif
	return true;
}

void ScreenCtl::setGamma(int brt, int temp)
{
#ifdef _WIN32
	setGDIGamma(brt, temp);
#else
	x11.setGamma(brt, temp);
#endif
}

uint64_t ScreenCtl::getResolution() {

#ifdef _WIN32
	const uint64_t width  = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
	const uint64_t height = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);
	return width * height;
#else
	return x11.getWidth() * x11.getHeight();
#endif
}

void ScreenCtl::getSnapshot(std::vector<uint8_t> &buf) noexcept
{
	using namespace std::this_thread;
	using namespace std::chrono;

#ifdef _WIN32
	if (useDXGI) {
		while (!dx.getDXGISnapshot(buf)) dx.restartDXGI();
	}
	else {
		getGDISnapshot(buf);
		sleep_for(milliseconds(cfg["polling_rate"]));
	}
#else
	x11.getSnapshot(buf);

	sleep_for(milliseconds(cfg["polling_rate"]));
#endif
}

void ScreenCtl::setInitialGamma(bool set_previous)
{
#ifdef _WIN32
	setGDIGamma(brt_slider_steps, 0);
#else
	x11.setInitialGamma(set_previous);
#endif
}
