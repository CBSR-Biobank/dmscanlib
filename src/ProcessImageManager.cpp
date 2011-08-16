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
#include "ProcessImageManager.h"
#include "BarcodeThread.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Decoder.h"
#include "BarcodeInfo.h"
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <map>

ProcessImageManager::ProcessImageManager(std::tr1::shared_ptr<Decoder> dec)
: decoder(dec) {
}

ProcessImageManager::~ProcessImageManager() {
}

void ProcessImageManager::addCells(std::vector<std::tr1::shared_ptr<PalletCell> > & cells,
		DecodedImageFunc func) {

}

//first is inclusive , last is exclusive
void ProcessImageManager::threadProcessRange(unsigned int first, unsigned int last) {
	for (unsigned int i = first; i < last; i++) {
		allThreads[i]->start();
	}

	for (unsigned int j = first; j < last; j++) {
		allThreads[j]->join();
	}

}

void ProcessImageManager::threadHandler(vector<BarcodeThread *> & threads) {

	unsigned int threadCount =
			(threads.size() < THREAD_NUM ) ? threads.size() : THREAD_NUM;

	unsigned int remainder = ((threads.size()) % threadCount);

	for (unsigned int i = 0; i < threads.size() - remainder; i += threadCount) {
		UA_DOUT(
				3,
				5,
				"Threads for barcodes finished: "<<i << "/" << i + threadCount-1);
		threadProcessRange(i, i + threadCount);

	}

	if (remainder > 0) {
		UA_DOUT(
				3,
				5,
				"Threads for barcodes finished(remainder): " << threads.size() - remainder << "/" << threads.size() - remainder-1 << " finished.");

		threadProcessRange(threads.size() - remainder, threads.size());
	}
}

void ProcessImageManager::generateBarcodes(Dib * dib,
		vector<vector<BarcodeInfo *> > & barcodeInfos) {
	vector<BarcodeThread *> threads;

	for (unsigned row = 0, rows = barcodeInfos.size(); row < rows; ++row) {
		for (unsigned col = 0, cols = barcodeInfos[row].size(); col < cols;
				++col) {

			BarcodeInfo * info = barcodeInfos[row][col];
			if (info == NULL) {
				continue;
			}

			std::tr1::shared_ptr<const CvRect> rect = info->getPreProcessBoundingBox();

//			BarcodeThread * thread = new BarcodeThread(decoder,
//					*barcodeInfos[row][col], ua::Logger::Instance().levelGet(3) >= 9);
//
//			allThreads.push_back(thread);
//			threads.push_back(thread);
		}
	}
	threadHandler(threads);
}
