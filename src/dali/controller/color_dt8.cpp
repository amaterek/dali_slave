/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "color_dt8.hpp"

#include <stdint.h>
#include <string.h>

// TC values in mired
#define TC_MIN 50
#define TC_MAX 1000
#define TC_STEP 50

namespace dali {
namespace controller {
namespace {

const PointXY tc2xy[] =  {
    { 16807, 16884 }, // 0 - minimal possible TC
    { 18392, 18893 }, // 1
    { 20382, 21047 }, // 2
    { 22617, 23043 }, // 3
    { 24933, 24691 }, // 4
    { 27200, 25910 }, // 5
    { 29325, 26700 }, // 6
    { 31261, 27111 }, // 7
    { 32992, 27214 }, // 8
    { 34517, 27086 }, // 9
    { 35859, 26793 }, // 10
    { 37031, 26394 }, // 11
    { 38073, 25925 }, // 12
    { 38980, 25430 }, // 13
    { 39799, 24916 }, // 14
    { 40521, 24411 }, // 15
    { 41177, 23916 }, // 16
    { 41762, 23446 }, // 17
    { 42291, 23001 }, // 18
    { 42779, 22574 }, // 19 - maximal possible TC
};

const Float kZero(0);
const Float kOne(65536);

bool isCalibrated(const Primary& p) {
  return (p.ty <= DALI_DT8_PRIMARY_TY_MAX) && (p.xy.x != DALI_DT8_MASK16) && (p.xy.x != DALI_DT8_MASK16);
}

bool xyToPrimary1(PointXY p, const PointXY prim_xy[], Float out[]) {
  out[0] = kOne;
  return (prim_xy[0].x != p.x) || (prim_xy[0].y != p.y);
}

Float float_abs(Float x) {
  if (x < kZero) {
    return kZero - x;
  }
  return x;
}

bool xyToPrimary2(PointXY p, const PointXY prim_xy[], Float out[]) {
  bool limitError = false;
  Float x1(prim_xy[0].x);
  Float x2(prim_xy[1].x);
  Float y1(prim_xy[0].y);
  Float y2(prim_xy[1].y);
  Float tmpx = x1 - x2;
  Float tmpy = y1 - y2;

  Float l1, l2;
  if (float_abs(tmpx) >= float_abs(tmpy)) {
    Float x = Float(p.x);
    l1 = (x - x2) / tmpx;
    l2 = (x1 - x) / tmpx;
  } else {
    Float y = Float(p.y);
    l1 = (y - y2) / tmpy;
    l2 = (y1 - y) / tmpy;
  }

  if (l1 < kZero) {
    l1 = kZero;
    limitError = true;
  } else if (l1 > kOne) {
    l1 = kOne;
    limitError = true;
  }

  if (l2 < kZero) {
    l2 = kZero;
    limitError = true;
  } else if (l2 > kOne) {
    l2 = kOne;
    limitError = true;
  }

  out[0] = l1;
  out[1] = l2;

  return limitError;
}

bool xyToPrimary3(PointXY p, const PointXY prim_xy[], Float out[]) {
  bool limitError = false;
  Float x(p.x);
  Float y(p.y);
  Float x1(prim_xy[0].x);
  Float y1(prim_xy[0].y);
  Float x2(prim_xy[1].x);
  Float y2(prim_xy[1].y);
  Float x3(prim_xy[2].x);
  Float y3(prim_xy[2].y);
  Float tmp = (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));

  if (tmp == Float(0)) {
    return true;
  }

  Float l1 = (x * (y2 - y3) + y * (x3 - x2) + x2 * y3 - x3 * y2) / tmp;
  if (l1 < kZero) {
    l1 = kZero;
    limitError = true;
  } else if (l1 > kOne) {
    l1 = kOne;
    limitError = true;
  }

  Float l2 = kZero - (x * (y1 - y3) + y * (x3 - x1) + x1 * y3 - x3 * y1) / tmp;
  if (l2 < kZero) {
    l2 = kZero;
    limitError = true;
  } else if (l2 > kOne) {
    l2 = kOne;
    limitError = true;
  }

  Float l3 = (x * (y1 - y2) + y * (x2 - x1) + x1 * y2 - x2 * y1) / tmp;
  if (l3 < kZero) {
    l3 = kZero;
    limitError = true;
  } else if (l3 > kOne) {
    l3 = kOne;
    limitError = true;
  }
  out[0] = l1;
  out[1] = l2;
  out[2] = l3;
  return limitError;
}

} // namespace

// static
uint16_t ColorDT8::primaryToTc(const Float level[], const Primary primary[], uint16_t nrOfPrimaries) {
  PointXY p = ColorDT8::primaryToXY(level, primary, nrOfPrimaries);

  uint16_t x = p.x;
  if (x <= tc2xy[0].x) {
    return TC_MIN;
  }
  if (x >= tc2xy[19].x) {
    return TC_MAX;
  }

  uint16_t a = 0;
  uint16_t b = 19;
  while (a + 1 < b) {
    uint16_t m = (a + b) / 2;
    uint16_t xx = tc2xy[m].x;
    if (x == xx) {
      a = b = m;
    } else if (x < xx) {
      b = m;
    } else {
      a = m;
    }
  }

  if (a == b) {
    return (a + 1) * TC_STEP;
  } else {
    uint16_t xx = x - tc2xy[a].x;
    uint16_t dx =  tc2xy[b].x -  tc2xy[a].x;
    return (a + 1) * TC_STEP + (xx * TC_STEP + dx / 2) / dx;
  }
}

// static
PointXY ColorDT8::tcToXY(uint16_t tc) {
  if (tc <= TC_MIN) {
    return tc2xy[0];
  }
  if (tc >= TC_MAX) {
    return tc2xy[19];
  }
  uint16_t i = tc / TC_STEP - 1;

  // linear approximation
  PointXY a = tc2xy[i];
  PointXY b = tc2xy[i + 1];
  int32_t r = tc % TC_STEP;
  PointXY result = a;
  result.x += (((int32_t)b.x - a.x) * r + TC_STEP / 2) / TC_STEP;
  result.y += (((int32_t)b.y - a.y) * r + TC_STEP / 2) / TC_STEP;
  return result;
}

// static
PointXY ColorDT8::primaryToXY(const Float level[], const Primary primary[], uint16_t nrOfPrimaries) {
  Float x(0);
  Float y(0);
  Float l(0);
  uint16_t min_ty = 65535;
  for (uint16_t i = 0; i < nrOfPrimaries; ++ i) {
    if (primary[i].ty < min_ty) {
      min_ty = primary[i].ty;
    }
  }

  for (uint16_t i = 0; i < nrOfPrimaries; ++ i) {
    const Primary& p = primary[i];
    if (isCalibrated(p)) {
      Float ty = level[i] * Float(primary[i].ty) / Float(min_ty);
      l +=  ty;
      x += Float(p.xy.x) * ty;
      y += Float(p.xy.y) * ty;
    }
  }
  PointXY result;
  if (l != kZero) {
    int32_t tx = x / l;
    result.x = tx <= 65535 ? tx : 65535;
    int32_t ty = y / l;
    result.y = ty <= 65535 ? ty : 65535;
  } else {
    // TODO how to handle this case
    result.x = 0;
    result.y = 0;
  }
  return result;
}

uint16_t findValidPrimaries(const Primary primary[], uint16_t nrOfPrimaries, PointXY primaryXY[], uint16_t primaryNr[]) {
  uint16_t n = 0;
  for (uint16_t i = 0; i < nrOfPrimaries; ++ i) {
    const Primary& p = primary[i];
    if (isCalibrated(p)) {
      primaryXY[n].x = p.xy.x;
      primaryXY[n].y = p.xy.y;
      primaryNr[n] = i;
      n++;
    }
  }
  return n;
}

// static
bool ColorDT8::xyToPrimary(PointXY xy, const Primary primary[], uint16_t nrOfPrimaries, Float level[]) {
  PointXY primaryXY[nrOfPrimaries];
  uint16_t primaryNr[nrOfPrimaries];
  uint16_t n = findValidPrimaries(primary, nrOfPrimaries, primaryXY, primaryNr);

  memset(level, 0, sizeof(uint16_t) * nrOfPrimaries);

  bool limitError = false;
  Float out[n];
  switch (n) {
  case 0:
    // no calibrated primary found
    break;

  case 1:
    limitError = xyToPrimary1(xy, primaryXY, out);
    break;

  case 2:
    limitError = xyToPrimary2(xy, primaryXY, out);
    break;

  case 3:
    limitError = xyToPrimary3(xy, primaryXY, out);
    break;

  default:
    // TODO find better solution ex. find nearest points
    n = 3;
    limitError =xyToPrimary3(xy, primaryXY, out);
    break;
  }

  uint16_t min_ty = 65535;
  for (uint16_t i = 0; i < nrOfPrimaries; ++ i) {
    if (primary[i].ty < min_ty) {
      min_ty = primary[i].ty;
    }
  }
  for (uint16_t i = 0; i < n; ++i) {
    uint16_t j = primaryNr[i];
    level[j] = (out[i] * Float(min_ty)) / Float(primary[j].ty);
  }
  return limitError;
}

} // controller
} // namespace dali
