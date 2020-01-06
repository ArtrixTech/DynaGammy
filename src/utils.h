/**
 * Copyright (C) 2019 Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef UTILS_H
#define UTILS_H

#include <array>
#include <vector>

int convertRange(int, int, int, int, int);
int kelvinToStep(int);

void setColors(int temp, std::array<double, 3> &c);

int calcBrightness(const std::vector<uint8_t> &buf);

// Windows functions

void getGDISnapshot(std::vector<uint8_t> &buf);
void setGDIGamma(int brightness, int temp);

void checkInstance();
void checkGammaRange();
void toggleRegkey(bool);

#endif // UTILS_H
