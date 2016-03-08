/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * All right reversed. Usage for commercial on not commercial
 * purpose without written permission is not allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_TEST_TEST_DT8_HPP_
#define DALI_TEST_TEST_DT8_HPP_

#ifdef DALI_TEST

#include "tests.hpp"

#include <dali/dali_dt8.hpp>

#ifdef DALI_DT8

namespace dali {

void unitTestsDT8();
void apiTestsDT8(CreateSlave createSlave);

} // namespace dali

#endif // DALI_DT8

#endif // DALI_TEST

#endif // DALI_TEST_TEST_DT8_HPP_
