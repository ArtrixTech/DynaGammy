/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifdef _WIN32
#include <Windows.h>
#endif

#include "utils.h"
#include "cfg.h"
#include "defs.h"

double lerp(double start, double end, double factor)
{
	return start * (1 - factor) + end * factor;
}

double normalize(double start, double end, double value)
{
	return (value - start) / (end - start);
}

double remap(double value, double from_min, double from_max, double to_min, double to_max)
{
	return lerp(to_min, to_max, normalize(from_min, from_max, value));
}

double interpTemp(int temp_step, size_t color_ch)
{
	return remap(temp_slider_steps - temp_step, 0, temp_slider_steps, ingo_thies_table[color_ch], 1);
};

double easeOutExpo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

double easeInOutQuad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1) {
		return c / 2 * t * t + b;
	} else {
		return -c / 2 * ((--t) * (t - 2) - 1) + b;
	}
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

void checkInstance()
{
	LOGV << "Checking for multiple instances";

	HANDLE hStartEvent = CreateEventA(nullptr, true, false, "Gammy");

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hStartEvent);
		hStartEvent = nullptr;

		LOGV << "Another instance of Gammy is running. Closing";
		exit(0);
	}
}
#endif
