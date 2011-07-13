#ifndef PROCESS_IMAGE_MANAGER_H_
#define PROCESS_IMAGE_MANAGER_H_
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
#include <vector>
#include <map>

using namespace std;

class Dib;
class BarcodeInfo;
class BarcodeThread;
class Decoder;

class ProcessImageManager {
public:
	ProcessImageManager(Decoder * decoder, double scanGap, unsigned squareDev,
			unsigned edgeThresh, unsigned corrections);
	~ProcessImageManager();

	void generateBarcodes(Dib * dib,
			vector<vector<BarcodeInfo *> > & barcodeInfos);

	Decoder * getDecoder() {
		return decoder;
	}

private:
	static const unsigned THREAD_NUM = 8;
	static const unsigned JOIN_TIMEOUT_SEC = 10;
	static const unsigned THRESHOLD_JOIN = 1;

	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;
	Decoder * decoder;
	vector<BarcodeThread *> allThreads;

	void threadHandler(vector<BarcodeThread *> & threads);

	void threadProcessRange(vector<BarcodeThread *> & threads, unsigned int first, unsigned int last);

};

#endif /* PROCESS_IMAGE_MANAGER_H_ */

