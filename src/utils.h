/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef UTILS_H
#define UTILS_H

#include <array>
#include <vector>
#include "stdint.h"

int calcBrightness(uint8_t *buf, uint64_t buf_sz, int bytes_per_pixel, int skip_mult);

double lerp(double start, double end, double factor);
double normalize(double start, double end, double value);
double remap(double value, double from_min, double from_max, double to_min, double to_max);
double interpTemp(int step, size_t color_ch);

// Windows functions
void getGDISnapshot(std::vector<uint8_t> &buf);
void setGDIGamma(int brightness, int temp);
void checkInstance();
void checkGammaRange();
void toggleRegkey(bool);

double easeOutExpo(double t, double b , double c, double d);
double easeInOutQuad(double t, double b, double c, double d);

#endif // UTILS_H
