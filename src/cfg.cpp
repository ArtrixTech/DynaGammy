/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#include "cfg.h"
#include "utils.h"
#include "defs.h"
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <Windows.h> // GetModuleFileNameW

std::wstring config::getExecutablePath()
{
	wchar_t buf[FILENAME_MAX] {};
	GetModuleFileNameW(nullptr, buf, FILENAME_MAX);
	std::wstring path(buf);

	std::wstring appname = L"gammy.exe";
	path.erase(path.find(appname), appname.length());

	path += L"gammyconf.txt";

	return path;
}
#else
std::string config::getPath()
{
	const char *home   = getenv("XDG_CONFIG_HOME");
	const char *format = "/";

	if (!home) {
		format = "/.config/";
		home = getenv("HOME");
	}

	std::stringstream ss;
	ss << home << format << config_name;

	return ss.str();
}
#endif

json getDefault()
{
	return
	{
		{"brt_auto", true},
		{"brt_fps", 60},
		{"brt_step", brt_slider_steps},
		{"brt_min", brt_slider_steps / 2},
		{"brt_max", brt_slider_steps},
		{"brt_offset", brt_slider_steps / 3},
		{"brt_speed", 2},
		{"brt_threshold", 8},
		{"brt_polling_rate", 100},
		{"brt_extend", false},

		{"temp_auto", false},
		{"temp_fps", 45},
		{"temp_step", 0},
		{"temp_high", max_temp_kelvin},
		{"temp_low", 3400},
		{"temp_speed", 60.0},
		{"temp_sunrise", "06:00:00"},
		{"temp_sunset", "16:00:00"},

		{"log_level", plog::warning},
		{"show_on_startup", false}
	};
}

json cfg = getDefault();

void config::read()
{
#ifdef _WIN32
	const std::wstring path = getExecutablePath();
#else
	const std::string path = config::getPath();
#endif

	std::ifstream file(path, std::fstream::in | std::fstream::app);

	if (!file.good() || !file.is_open()) {
		LOGE << "Unable to open config at path: " << path;
		return;
	}

	LOGD << "Reading from: " << path;

	// Seek file end
	file.seekg(0, std::ios::end);

	// If end position is 0, create the file
	if (file.tellg() == 0) {
		config::write();
		return;
	}

	// Seek file start
	file.seekg(0);

	json tmp;

	try {
		file >> tmp;
	} catch (json::exception &e) {
		LOGE << e.what() << " - Resetting config...";
		cfg = getDefault();
		config::write();
		return;
	}

	cfg.update(tmp);

	LOGV << "Config parsed";
}

void config::write()
{
#ifdef _WIN32
	const std::wstring path = getExecutablePath();
#else
	const std::string path = getPath();
#endif

	std::ofstream file(path, std::ofstream::out);

	if (!file.good() || !file.is_open()) {
		LOGE << "Unable to open config file";
		return;
	}

	LOGD << "Writing to: " << path;

	try {
		file << std::setw(4) << cfg;
	} catch (json::exception &e) {
		LOGE << e.what() << " id: " << e.id;
		return;
	}

	LOGV << "Config set";
}
