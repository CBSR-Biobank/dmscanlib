#ifndef __INC_UaAssert_h
#define __INC_UaAssert_h

/*****************************************************************************
 *
 * Name: $Id: UaDebug.h,v 1.5 2007/07/18 23:37:36 octopus Exp $
 *
 *-----------------------------------------------------------------------------
 *
 * The information contained herein is proprietary and confidential to Alberta
 * Ingenuity Centre For Machine Learning (AICML) and describes aspects of AICML
 * products and services that must not be used or implemented by a third party
 * without first obtaining a license to use or implement.
 *
 * Copyright 2006 Alberta Ingenuity Centre For Machine Learning.
 *
 *-----------------------------------------------------------------------------
 *
 * Modification History:
 *
 * Date           Name             Description
 * --------       ---------------  --------------------------------------------
 * Oct25/2006     N.Loyola         Added this comment header.
 *
 *****************************************************************************/

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
