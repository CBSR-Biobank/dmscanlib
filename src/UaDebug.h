#ifndef __INC_bkDebug_h
#define __INC_bkDebug_h

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

#include "Singleton.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef UA_HAVE_DEBUG

namespace ua {
    class DebugSink {
    public:
        DebugSink () : enableHeader_m(false) {};
        virtual ~DebugSink () {};

        virtual void write (const std::string& str) = 0;

        virtual void header(std::string & str_r) {
            standardHeader(str_r);
        }
        // By default, we emit a standard header when headers are enabled

        void showHeader (bool state) {  enableHeader_m = state;}
        // Set the state of whether a header is written

    protected:
        void standardHeader(std::string & str);

        bool enableHeader_m; // true writes header when buffer is flushed
    };

    class DebugSinkNullImpl : public DebugSink  {
    public:
        DebugSinkNullImpl() {};
        virtual void write (const std::string& str) {}
    };

    typedef Loki::SingletonHolder<DebugSinkNullImpl> DebugSinkNull;


    class DebugSinkStdoutImpl : public DebugSink {
    public:
        DebugSinkStdoutImpl () {};

        virtual void write (const std::string& str);
    };

    typedef Loki::SingletonHolder<DebugSinkStdoutImpl> DebugSinkStdout;

    class DebugSinkFileImpl : public DebugSinkStdoutImpl {
    public:
        DebugSinkFileImpl() {};
        virtual ~DebugSinkFileImpl() {};

        virtual void write (const std::string& str);

        // Set/change our file name. The stream is flushed before the
        // file name is changed.
        void setFile (const std::string& file);

    private:
        std::string file_m;
    };

    typedef Loki::SingletonHolder<DebugSinkFileImpl> DebugSinkFile;

    template<class T, class Tr = std::char_traits<T>,
             class A = std::allocator<T> >
    class DebugStringBuf : public std::basic_stringbuf<T, Tr, A> {
    public:
        DebugStringBuf (DebugSink& s = DebugSinkNull::Instance())
                : sink_m (&s) {
        }
        ~DebugStringBuf () {
            if (std::basic_stringbuf<T, Tr, A>::str().size() > 0) {
                std::cerr << "debug buffer contents: " << std::endl
                          << std::basic_stringbuf<T, Tr, A>::str()
                          << std::endl;
            }
        }

        int sync() {
            sink_m->write(std::basic_stringbuf<T, Tr, A>::str());

            // Clear the string buffer
            std::basic_stringbuf<T, Tr, A>::str(std::basic_string<T, Tr, A>());
            return 0;
        }

        void sink (DebugSink& s) {
            // Change our sink
            sink_m = &s;
        }

    private:
        DebugSink* sink_m;
    };

    class DebugImpl {
    public:
        DebugImpl ();

        void levelInc(unsigned subsys);
        void levelDec(unsigned subsys);
        void levelSet(unsigned subsys, unsigned level);
        unsigned levelGet(unsigned subsys);

        void subSysHeaderSet(unsigned subsys, std::string header);
        std::string & subSysHeaderGet(unsigned subsys);

        // Turns off all debugging
        void reset();

        // Returns true if the subsystem level is enabled for debugging.
        bool isDebug (unsigned subsys, unsigned level);

        static const unsigned maxSubSys_m = 32;
        static const unsigned allSubSys_m = maxSubSys_m;

    private:
        static const unsigned maxLevel_m = 9;
        unsigned levels_am[maxSubSys_m];
        std::string headers_am[maxSubSys_m];
    };

    typedef Loki::SingletonHolder<DebugImpl> Debug;

    extern DebugStringBuf<char> debugstream;
    extern std::ostream cdebug;
}

#define UA_DEBUG(statements)                     \
    do {                                         \
        statements;                              \
    }                                            \
    while(0)

#define UA_DOUT(subsys, level, display)                                 \
    do {                                                                \
        if (ua::Debug::Instance().isDebug(subsys, level)) {             \
            ua::cdebug << ua::Debug::Instance().subSysHeaderGet(subsys) \
                       << " "                                           \
                       << ua::Debug::Instance().levelGet(subsys)        \
                       << " "                                           \
                       << display << std::endl;                         \
        }                                                               \
    }                                                                   \
    while(0)

/**
 * When an cond is false, an error message is generated and the program exits.
 */
#define UA_ASSERT(cond) assert ( (cond) )

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

#define UA_DOUT(subsys, level, statements)

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

#endif  /* DEBUG */

#endif /* __INC_bkDebug_h */
