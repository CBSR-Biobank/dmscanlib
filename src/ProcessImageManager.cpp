
#include "ProcessImageManager.h"
#include "BarcodeThread.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Decoder.h"
#include "BarcodeInfo.h"

#ifdef WIN32
#include <windows.h>
#endif

ProcessImageManager::ProcessImageManager(double scanGap, unsigned squareDev,
					 unsigned edgeThresh,
					 unsigned corrections)
{
	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
}

void ProcessImageManager::threadHandler(vector < BarcodeInfo * > & barcodeInfos,
		vector < BarcodeThread * > & threads, unsigned threshold)
{
	time_t timeStart, timeEnd;

	time(&timeStart);

	vector < BarcodeThread * >tempthreads;

	while (1) {

		for (unsigned j = 0; j < threads.size(); j++) {
			BarcodeThread & thread = *threads[j];
			if (thread.isFinished()) {
				vector < BarcodeInfo * > & barcodes =
				    thread.getBarcodes();

				barcodeInfos.reserve(barcodeInfos.size() + barcodes.size());
				barcodeInfos.insert(barcodeInfos.end(),
						barcodes.begin(), barcodes.end());

			} else {
				tempthreads.push_back(&thread);
			}
		}
		threads = tempthreads;
		tempthreads.clear();

		if (threads.size() < threshold)
			break;
		else
			sleep(1);

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
		vector <CvRect> *blobVector,
		vector <BarcodeInfo * > & barcodeInfos)
{

	UA_DOUT(3, 5,
		"getTubeBlobs found: " << blobVector->size() << " blobs.");

	vector < BarcodeThread * >threads;

	for (int i = 0; i < (int)blobVector->size(); i++) {

		/*---thread controller (limit # threads to THREAD_NUM)----*/
		threadHandler(barcodeInfos, threads, THREAD_NUM);

		Dib tmp;
		tmp.crop(*dib,
			  (*blobVector)[i].x,
			  (*blobVector)[i].y,
			  (*blobVector)[i].x + (*blobVector)[i].width,
			  (*blobVector)[i].y + (*blobVector)[i].height);

		BarcodeThread *thread = new BarcodeThread(this->scanGap,
							  this->squareDev,
							  this->edgeThresh,
							  this->corrections,
							  (*blobVector)[i],
							  tmp);

		threads.push_back(thread);
		thread->run();

	}

	/*---join---*/
	threadHandler(barcodeInfos, threads, THRESHOLD_JOIN);

	//TODO add upper bound to blob size
	vector < BarcodeInfo * >tempBarcodes;
	for (unsigned y = 0, n = barcodeInfos.size(); y < n; y++) {

		bool uniqueBarcode = true;
		for (unsigned x = 0; x < tempBarcodes.size(); x++)
			if (tempBarcodes[x]->Equals(barcodeInfos[y])) {
				uniqueBarcode = false;
				break;
			}

		if (uniqueBarcode)
			tempBarcodes.push_back(barcodeInfos[y]);
	}

	barcodeInfos.clear();
	barcodeInfos = tempBarcodes;
}
