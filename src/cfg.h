/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef CFG_H
#define CFG_H

#include "utils.h"
#include "json.hpp"

using json = nlohmann::json;

extern json cfg;

namespace config {
#ifdef _WIN32
std::wstring getPath();
#else
std::string  getPath();
#endif
void read();
void write();
}

#endif // CFG_H
