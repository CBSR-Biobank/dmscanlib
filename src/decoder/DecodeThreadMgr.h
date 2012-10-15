#ifndef __INC_PROCESS_IMAGE_MANAGER_H
#define __INC_PROCESS_IMAGE_MANAGER_H

/*
 Dmscanlib is a software library and stand alone application that scans
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

#include <dmtx.h>
#include <string>
#include <memory>

#ifdef _VISUALC_
#   include <functional>
#else
#   include <tr1/functional>
#endif

using namespace std;

class WellDecoder;

class DecodeThreadMgr {
public:
	DecodeThreadMgr();
	~DecodeThreadMgr();

	void decodeWells(vector<unique_ptr<WellDecoder> > & wellDecoders);

private:
	static const unsigned THREAD_NUM;

	void threadHandler();
	void threadProcessRange(unsigned int first, unsigned int last);

	vector<WellDecoder *> allThreads;
	unsigned numThreads;
};

#endif /* __INC_PROCESS_IMAGE_MANAGER_H */

