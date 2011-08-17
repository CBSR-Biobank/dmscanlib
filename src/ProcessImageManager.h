#ifndef __INC_PROCESS_IMAGE_MANAGER_H
#define __INC_PROCESS_IMAGE_MANAGER_H

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
/*
 * processImageManager.h
 *		Created on: 6-Jul-2010
 *		Author: Thomas Polasek
 */

#include "cv.h"
#include "dmtx.h"

//#include <vector>
#include <string>
//#include <map>

#ifdef _VISUALC_
#   include <memory>
#   include <functional>
#else
#   include <tr1/memory>
#   include <tr1/functional>
#endif

using namespace std;

class Dib;
class PalletCell;
class DecodeInfo;
class BarcodeThread;
class Decoder;

class ProcessImageManager {
public:
	ProcessImageManager(std::tr1::shared_ptr<Decoder> decoder);
	~ProcessImageManager();

	void decodeCells(std::vector<std::tr1::shared_ptr<PalletCell> > & cells);

	void decodeCallback(std::tr1::shared_ptr<PalletCell> cell,
			std::string & msg, CvPoint(&corners)[4]);

private:
	static const unsigned THREAD_NUM;

	void threadHandler();
	void threadProcessRange(unsigned int first, unsigned int last);

	std::tr1::shared_ptr<Decoder> decoder;
	vector<std::tr1::shared_ptr<BarcodeThread> > allThreads;
	unsigned numThreads;
};

#endif /* __INC_PROCESS_IMAGE_MANAGER_H */

