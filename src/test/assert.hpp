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

#ifndef TEST_H_
#define TEST_H_

void testLogFailure(const char* str);

#define STR_LINE(x) #x
#define STR_LINE_(x) STR_LINE(x)

#define TEST_FALIURE(cond) testLogFailure("") //testLogFailure(__FILE__ ":" STR_LINE_(__LINE__))

#define TEST_SUCCESS(cond)

#define TEST_ASSERT(cond) \
  do { \
    if (!(cond)) { \
      TEST_FALIURE(cond); \
    } else { \
      TEST_SUCCESS(cond); \
    } \
  } while(0)

#endif // TEST_H_
