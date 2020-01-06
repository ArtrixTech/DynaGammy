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
#endif

json setDefault()
{
	return
	{
		{"brightness", 255},
		{"min_br", 192 },
		{"max_br", 255 },
		{"offset", 78 },
		{"speed", 3 },
		{"threshold", 32 },
		{"polling_rate", 100 },
		{"temp_step", 0 },
		{"temp_high", max_temp_kelvin },
		{"temp_low", 3400 },
		{"temp_state", 0 },
		{"time_start", "17:00:00" },
		{"time_end", "06:00:00" },
		{"auto_br", true },
		{"auto_temp", false },
		{"extend_br", false },
		{"log_lvl", plog::warning }
	};
};

json cfg = setDefault();

void save()
{
#ifdef _WIN32
	static const std::wstring path = getExecutablePath();
#else
	static const std::string path = getConfigPath();
#endif

	if(path.empty())
	{
		LOGE << "Invalid config path";
		return;
	}

	std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);

	if(!file.good() || !file.is_open())
	{
		LOGE << "Unable to open config file";
		return;
	}

	try
	{
		file << std::setw(4) << cfg;
	}
	catch (json::exception &e)
	{
		LOGE << e.what() << " id: " << e.id;
		return;
	}

	LOGD << "Settings saved";
}

void read()
{
#ifdef _WIN32
	static const std::wstring path = getExecutablePath();
#else
	static const std::string path = getConfigPath();
#endif

	std::fstream file(path, std::fstream::in | std::fstream::out | std::fstream::app);

	if(!file.good() || !file.is_open())
	{
		LOGE << "Unable to open config.";
		return;
	}

	file.seekg(0, std::ios::end);

	if(file.tellg() == 0)
	{
		LOGW << "Config empty. Creating one.";
		save();
		return;
	}

	file.seekg(0);

	try
	{
		file >> cfg;
	}
	catch (json::exception &e)
	{
		LOGE << e.what();
		LOGI << "Resetting config.";

		cfg = setDefault();
		save();

		return;
	}

	LOGD << "Settings parsed";
}

#ifndef _WIN32
std::string getConfigPath()
{
	const char *xdg_cfg_home;
	const char *home;

	std::string path;
	std::string cfg_filename = "gammy";

	xdg_cfg_home = getenv("XDG_CONFIG_HOME");

	if (xdg_cfg_home)
	{
		path = std::string(xdg_cfg_home) + '/';
	}
	else
	{
		home = getenv("HOME");
		if(!home) return "";

		path = std::string(home) + "/.config/";
	}

	path += cfg_filename;

	LOGI << "Config path: " + path;

	return path;
}
#else
std::wstring getExecutablePath()
{
    wchar_t buf[FILENAME_MAX] {};
    GetModuleFileNameW(nullptr, buf, FILENAME_MAX);
    std::wstring path(buf);

    std::wstring appname = L"gammy.exe";
    path.erase(path.find(appname), appname.length());

    path += L"gammysettings.cfg";

    return path;
}
#endif


