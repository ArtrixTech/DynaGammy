/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef CFG_H
#define CFG_H

#include "utils.h"
#include "json.hpp"

using json = nlohmann::json;

extern json cfg;

auto getConfigPath()     -> std::string;
auto getExecutablePath() -> std::wstring;

void read();
void write();

#endif // CFG_H
