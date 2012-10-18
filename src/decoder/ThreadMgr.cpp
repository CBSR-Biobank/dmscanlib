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

#include "ThreadMgr.h"
#include "DmScanLib.h"
#include "WellDecoder.h"

#include <algorithm>
#include <vector>
#include <memory>
#include <glog/logging.h>

#ifdef WIN32
#   include <windows.h>
#   undef ERROR
#endif

namespace dmscanlib {

namespace decoder {

const unsigned ThreadMgr::THREAD_NUM = 8;

ThreadMgr::ThreadMgr() {
}

ThreadMgr::~ThreadMgr() {
}

   void ThreadMgr::decodeWells(std::vector<std::unique_ptr<WellDecoder> > & wellDecoders) {
	numThreads = wellDecoders.size();
	allThreads.resize(numThreads);

	for (unsigned i = 0; i < numThreads; ++i) {
		allThreads[i] = wellDecoders[i].get();
	}

	threadHandler();
}

void ThreadMgr::threadHandler() {
	unsigned first = 0;
	unsigned last = std::min(numThreads, THREAD_NUM);

	do {
		threadProcessRange(first, last);
		VLOG(2) << "Threads for cells finished: " << first << "/" << last - 1;

		first = last;
		last = std::min(last + THREAD_NUM, numThreads);
	} while (first < numThreads);
}

//first is inclusive , last is exclusive
void ThreadMgr::threadProcessRange(unsigned first, unsigned last) {
	for (unsigned int i = first; i < last; i++) {
		allThreads[i]->start();
	}

	for (unsigned int j = first; j < last; j++) {
		allThreads[j]->join();
	}
}

} /* namespace */

}/* namespace */
