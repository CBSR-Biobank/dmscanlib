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

#ifdef WIN32
#include <windows.h>
#define SLEEP Sleep
#else 
#define SLEEP sleep
#endif

#include <map>

ProcessImageManager::ProcessImageManager(Decoder * decoder, double scanGap,
		unsigned squareDev, unsigned edgeThresh, unsigned corrections)
{
	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
	this->decoder = decoder;
}

ProcessImageManager::~ProcessImageManager() {
	for (unsigned i = 0, n = allThreads.size(); i < n; ++i) {
		delete allThreads[i];
	}
}


void ProcessImageManager::threadHandler(vector < BarcodeThread * > & threads,
		unsigned threshold)
{
	time_t timeStart, timeEnd;

	time(&timeStart);

	vector < BarcodeThread * >unfinishedThreads;

	while (1) {

		for (unsigned j = 0; j < threads.size(); j++) {
			BarcodeThread & thread = *threads[j];
			if (!thread.isFinished()) {
				unfinishedThreads.push_back(&thread);
			}
		}
		threads = unfinishedThreads;
		unfinishedThreads.clear();

		if (threads.size() < threshold)
			break;
		else
			SLEEP(1);

		/*----join----*/
		if (threshold == THRESHOLD_JOIN) {
			time(&timeEnd);
			if (difftime(timeEnd, timeStart) >= JOIN_TIMEOUT_SEC) {
				UA_DOUT(3, 1,
					"Error:: Some threads have timed out.");
				break;
			}
		}

	}
}

void ProcessImageManager::generateBarcodes(Dib * dib,
		vector <CvRect> & blobVector,
		vector <BarcodeInfo * > & barcodeInfos)
{

	UA_DOUT(3, 5,
		"getTubeBlobs found: " << blobVector.size() << " blobs.");

	vector < BarcodeThread * > threads;

	for (unsigned i = 0, n = blobVector.size(); i < n; i++) {

		/*---thread controller (limit # threads to THREAD_NUM)----*/
		threadHandler(threads, THREAD_NUM);

		Dib croppedDib;
		croppedDib.crop(*dib,
			  blobVector[i].x,
			  blobVector[i].y,
			  blobVector[i].x + blobVector[i].width,
			  blobVector[i].y + blobVector[i].height);

		BarcodeThread *thread = new BarcodeThread(this, scanGap,
							  squareDev,
							  edgeThresh,
							  corrections,
							  blobVector[i],
							  croppedDib);

		allThreads.push_back(thread);
		threads.push_back(thread);
		thread->run();
	}

	/*---join---*/
	threadHandler(threads, THRESHOLD_JOIN);
}
