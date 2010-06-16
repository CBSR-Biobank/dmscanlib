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

namespace ua {
    class LoggerSink {
    public:
    	LoggerSink () : enableHeader_m(false) {};
        virtual ~LoggerSink () {};

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

    class LoggerSinkNullImpl : public LoggerSink  {
    public:
        LoggerSinkNullImpl() {};
        virtual void write (const std::string& str) {}
    };

    typedef Loki::SingletonHolder<LoggerSinkNullImpl> LoggerSinkNull;


    class LoggerSinkStdoutImpl : public LoggerSink {
    public:
        LoggerSinkStdoutImpl () {};

        virtual void write (const std::string& str);
    };

    typedef Loki::SingletonHolder<LoggerSinkStdoutImpl> LoggerSinkStdout;

    class LoggerSinkFileImpl : public LoggerSinkStdoutImpl {
    public:
        LoggerSinkFileImpl() {};
        virtual ~LoggerSinkFileImpl() {};

        virtual void write (const std::string& str);

        // Set/change our file name. The stream is flushed before the
        // file name is changed.
        void setFile (const std::string& file);

    private:
        std::string file_m;
    };

    typedef Loki::SingletonHolder<LoggerSinkFileImpl> LoggerSinkFile;

    template<class T, class Tr = std::char_traits<T>,
             class A = std::allocator<T> >
    class LoggerStringBuf : public std::basic_stringbuf<T, Tr, A> {
    public:
        LoggerStringBuf (LoggerSink& s = LoggerSinkNull::Instance())
                : sink_m (&s) {
        }
        ~LoggerStringBuf () {
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

        void sink (LoggerSink& s) {
            // Change our sink
            sink_m = &s;
        }

    private:
        LoggerSink* sink_m;
    };

    class LoggerImpl {
    public:
        LoggerImpl ();

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
        std::vector<std::string> headers_m;
    };

    typedef Loki::SingletonHolder<LoggerImpl> Logger;

    extern LoggerStringBuf<char> logstream;
    extern std::ostream cdebug;
}

#define UA_DOUT(subsys, level, display)                                 \
    do {                                                                \
        if (ua::Logger::Instance().isDebug(subsys, level)) {             \
            ua::cdebug << ua::Logger::Instance().subSysHeaderGet(subsys) \
                       << " "                                           \
                       << level                                         \
                       << " "                                           \
                       << display << std::endl;                         \
        }                                                               \
    }                                                                   \
    while(0)

#endif /* __INC_bkDebug_h */
