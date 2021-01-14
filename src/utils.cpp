/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
#include <Windows.h>
#endif

#include "utils.h"
#include "cfg.h"
#include "defs.h"

int calcBrightness(uint8_t *buf, uint64_t buf_sz, int bytes_per_pixel, int skip_mult)
{
	const uint64_t res = buf_sz / bytes_per_pixel;

	uint64_t r = 0;
	uint64_t g = 0;
	uint64_t b = 0;

	const int inc = bytes_per_pixel * skip_mult;

	for (uint64_t i = 0; i < buf_sz; i += inc) {
		r += buf[i + 2];
		g += buf[i + 1];
		b += buf[i];
	}

	int lum = (r * 0.2126 + g * 0.7152 + b * 0.0722) * skip_mult / res;

	return lum;
}

double lerp(double x, double a, double b)
{
	return (1 - x) * a + x * b;
}

double normalize(double x, double a, double b)
{
	return (x - a) / (b - a);
}

double remap(double x, double a, double b, double ay, double by)
{
	return lerp(normalize(x, a, b), ay, by);
}

double interpTemp(int temp_step, size_t color_ch)
{
	return remap(temp_steps_max - temp_step, 0, temp_steps_max, ingo_thies_table[color_ch], 1);
};

double easeOutExpo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

double easeInOutQuad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1)
		return c / 2 * t * t + b;
	else
		return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
};

#ifdef _WIN32
void checkGammaRange()
{
	LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM";
	LSTATUS s;

	s = RegGetValueW(HKEY_LOCAL_MACHINE, subKey, L"GdiICMGammaRange", RRF_RT_REG_DWORD, nullptr, nullptr, nullptr);

	if (s == ERROR_SUCCESS) {
		LOGI << "Gamma regkey already set";
		return;
	}

	LOGW << "Gamma regkey not set. Creating one...";

	HKEY hKey;

	s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);

	if (s == ERROR_SUCCESS) {
		LOGI << "Gamma registry key created";
		DWORD val = 256;
		s = RegSetValueExW(hKey, L"GdiICMGammaRange", 0, REG_DWORD, LPBYTE(&val), sizeof(val));

		if (s == ERROR_SUCCESS) {
			MessageBoxW(nullptr, L"Gammy has extended the brightness range. Restart to apply the changes.", L"Gammy", 0);

			LOGI << "Gamma regvalue set";
		}
		else {
			LOGE << "Error when setting Gamma registry value";
		}
	} else {
		LOGE << "** Error when creating/opening gamma regkey";
		if (s == ERROR_ACCESS_DENIED) {
			LOGE << "** ACCESS_DENIED";
		}
	}

	if (hKey)
		RegCloseKey(hKey);
}

void toggleRegkey(bool checked)
{
	LSTATUS s;
	HKEY hKey = nullptr;
	LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

	if (checked) {
		WCHAR path[MAX_PATH + 3], tmpPath[MAX_PATH + 3];
		GetModuleFileName(nullptr, tmpPath, MAX_PATH + 1);
		wsprintfW(path, L"\"%s\"", tmpPath);

		s = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);

		if (s == ERROR_SUCCESS) {
			s = RegSetValueExW(hKey, L"Gammy", 0, REG_SZ, LPBYTE(path), int((wcslen(path) * sizeof(WCHAR))));
			if (s == ERROR_SUCCESS)
				LOGI << "Startup regkey set";
			else
				LOGE << "Failed to set startup regvalue";
		}
		else {
			LOGE << "Failed to open startup regkey";
		}
	} else {
		s = RegDeleteKeyValueW(HKEY_CURRENT_USER, subKey, L"Gammy");
		if (s == ERROR_SUCCESS)
			LOGI << "Run at startup unset";
		else
			LOGE << "Failed to delete startup regkey";
	}

	if (hKey)
		RegCloseKey(hKey);
}
#endif

bool alreadyRunning()
{
#ifdef _WIN32
	HANDLE ev = CreateEventA(nullptr, true, false, "Gammy");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hStartEvent);
		ev = nullptr;
		return true;
	}
#else
	static int fd;
	struct flock fl;
	fl.l_type   = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 1;

	fd = open("/tmp/gammy.lock", O_WRONLY | O_CREAT, 0666);

	if (fd == -1 || fcntl(fd, F_SETLK, &fl) == -1)
		return true;
#endif
	return false;
}
