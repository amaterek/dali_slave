/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_CONFIG_H_
#define DALI_CONFIG_H_

#define DALI_VERSION 1
#define DALI_DEVICE_TYPE 8

#define DALI_BANKS 3
#define DALI_BANK0_ADDR 0   // Bank 0 (16 bytes) according to 62386-102
#define DALI_BANK1_ADDR 16  // Bank 1 (16 bytes) according to 62386-102
#define DALI_BANK2_ADDR 32  // Bank 2 (28 bytes) data 62386-102 (26 bytes)
#define DALI_BANK3_ADDR 60

#define DALI_PHISICAL_MIN_LEVEL 1

#endif // DALI_CONFIG_H_
