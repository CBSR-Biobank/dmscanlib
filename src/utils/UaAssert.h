#ifndef __INC_UaAssert_h
#define __INC_UaAssert_h
/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * The UA_ASSERT macros contained here are used for debugging purposes.
 *
 * When an ASSERT fails, an error message is generated and the program exits.
 */

#include <assert.h>
#include <iostream>
#include <sstream>

#ifdef _DEBUG

#define UA_DEBUG(statements)                     \
    do {                                         \
        statements;                              \
    }                                            \
    while(0)

/**
 * When an cond is false, an error message is generated and the program exits.
 */
#define UA_ASSERT(cond) assert ( (cond) )

/**
 * When an cond is false, an error message is generated and the program exits.
 */
#define UA_ASSERT_NOT_NULL(ptr) assert ( (ptr) != NULL )

/**
 * When an cond is false, an error message is generated and the program exits.
 */
#define UA_ASSERTS(cond, msg)                    \
    do {                                         \
        if (! (cond) ) {                         \
            std::cerr << "ASSERT FAILED!" << std::endl  \
                      << __FILE__ << ":" << __LINE__    \
                      << ": " << msg << std::endl;      \
            abort();                             \
        }                                        \
    }                                            \
    while(0)

/**
 * Outputs an error message and halts the program.
 */
#define UA_ERROR(msg)                                                   \
    do {                                                                \
        std::cerr << "ERROR!" << std::endl << __FILE__ << ":" << __LINE__ \
                  << ": " << msg << std::endl;                          \
        abort();                                                        \
    }                                                                   \
    while(0)


/**
 * Outputs an warning message and does not terminate the program.
 */
#define UA_WARN(msg)                                                    \
    do {                                                                \
        std::cerr << "WARNING: " << __FILE__ << ":" << __LINE__         \
                  << ": " << msg << std::endl;                          \
    }                                                                   \
    while(0)

#else /* _DEBUG */

#define UA_DEBUG(statements)

#define UA_ASSERT_NOT_NULL(ptr)

/**
 * Does nothing if load is not compiled in DEBUG mode.
 */
#define UA_ASSERT(cond)

/**
 * Does nothing if load is not compiled in DEBUG mode.
 */
#define UA_ASSERTS(cond, msg)

/**
 * Does nothing if load is not compiled in DEBUG mode.
 */
#define UA_ERROR(msg)

/**
 * Does nothing if load is not compiled in DEBUG mode.
 */
#define UA_WARN(msg)

#endif  /* UA_HAVE_DEBUG */

#endif /* __INC_UaAssert_h */
