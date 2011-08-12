#ifndef __INC_bkDebug_h
#define __INC_bkDebug_h
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

#include <OpenThreads/Mutex>

#include <assert.h>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef USE_NVWA
#   include "debug_new.h"
#endif

#include "Singleton.h"

namespace ua {
class LoggerSink {
public:
	LoggerSink() :
		enableHeader_m(false) {
	}
	;
	virtual ~LoggerSink() {
	}
	;

	virtual void write(const std::string& str) = 0;

	virtual void header(std::string & str_r) {
		standardHeader(str_r);
	}
	// By default, we emit a standard header when headers are enabled

	void showHeader(bool state) {
		enableHeader_m = state;
	}
	// Set the state of whether a header is written

protected:
	void standardHeader(std::string & str);

	bool enableHeader_m; // true writes header when buffer is flushed
	OpenThreads::Mutex mutex;
};

class LoggerSinkNullImpl: public LoggerSink {
public:
	LoggerSinkNullImpl() {
	}
	;
	virtual void write(const std::string& str) {
	}
};

typedef Loki::SingletonHolder<LoggerSinkNullImpl> LoggerSinkNull;

class LoggerSinkStdoutImpl: public LoggerSink {
public:
	LoggerSinkStdoutImpl() {
	}
	;

	virtual void write(const std::string& str);
};

typedef Loki::SingletonHolder<LoggerSinkStdoutImpl> LoggerSinkStdout;

class LoggerSinkFileImpl: public LoggerSinkStdoutImpl {
public:
	LoggerSinkFileImpl() {
	}
	;
	virtual ~LoggerSinkFileImpl() {
	}
	;

	virtual void write(const std::string& str);

	// Set/change our file name. The stream is flushed before the
	// file name is changed.
	void setFile(const std::string& file);

private:
	std::string file_m;
};

typedef Loki::SingletonHolder<LoggerSinkFileImpl> LoggerSinkFile;

template<class T, class Tr = std::char_traits<T>, class A = std::allocator<T> >
class LoggerStringBuf: public std::basic_stringbuf<T, Tr, A> {
public:
	LoggerStringBuf(LoggerSink& s = LoggerSinkNull::Instance()) :
		sink_m(&s) {
	}
	~LoggerStringBuf() {
		if (std::basic_stringbuf<T, Tr, A>::str().size() > 0) {
			std::cerr << "debug buffer contents: " << std::endl
					<< std::basic_stringbuf<T, Tr, A>::str() << std::endl;
		}
	}

	int sync() {
		sink_m->write(std::basic_stringbuf<T, Tr, A>::str());

		// Clear the string buffer
		std::basic_stringbuf<T, Tr, A>::str(std::basic_string<T, Tr, A>());
		return 0;
	}

	void sink(LoggerSink& s) {
		// Change our sink
		sink_m = &s;
	}

private:
	LoggerSink* sink_m;
};

class LoggerImpl {
public:
	LoggerImpl();

	void levelInc(unsigned subsys);
	void levelDec(unsigned subsys);
	void levelSet(unsigned subsys, unsigned level);
	unsigned levelGet(unsigned subsys);

	void subSysHeaderSet(unsigned subsys, std::string header);
	std::string & subSysHeaderGet(unsigned subsys);

	// Turns off all debugging
	void reset();

	// Returns true if the subsystem level is enabled for debugging.
	bool isDebug(unsigned subsys, unsigned level);

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

/*
 UA_DOUT is not thread safe.
 */
#define UA_DOUT(subsys, level, display)                                      \
		do {                                                                 \
			if (ua::Logger::Instance().isDebug(subsys, level)) {             \
				ua::cdebug << ua::Logger::Instance().subSysHeaderGet(subsys) \
						   << " "                                            \
						   << level                                          \
						   << " "                                            \
						   << display << std::endl;                          \
			}                                                                \
		}                                                                    \
		while(0)


#endif /* __INC_bkDebug_h */
