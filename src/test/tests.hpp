/*/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * All right reversed. Usage for commercial on not commercial
 * purpose without written permission is not allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_DALI_TEST_HPP_
#define DALI_DALI_TEST_HPP_

#ifdef DALI_TEST

#include <dali/slave.hpp>

namespace dali {

typedef Slave* (*CreateSlave)(IMemory* memoryDriver, ILamp* lampDriver, IBus* busDriver, ITimer* timer);

void unitTests();
void apiTests(CreateSlave createSlave);

} // namespace dali

#endif // DALI_TEST

#endif // DALI_DALI_TEST_HPP_
