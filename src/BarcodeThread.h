#ifndef BARCODE_THREAD_H_
#define BARCODE_THREAD_H_

#include "cv.h"

#include <vector>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

using namespace std;

class Dib;
class BarcodeInfo;

class BarcodeThread : public OpenThreads::Thread {
public:
	BarcodeThread(double scanGap, unsigned squareDev, unsigned edgeThresh,
		      unsigned corrections, CvRect croppedOffset, Dib & dib);

	virtual ~ BarcodeThread() {
	};

	virtual void run();

	bool isFinished();
	vector < BarcodeInfo * > & getBarcodes();

 private:
	OpenThreads::Mutex quitMutex;
	volatile bool quitFlag;

	vector < BarcodeInfo * > barcodeInfo;

	Dib & dib;
	CvRect croppedOffset;
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;

};

#endif /* BARCODE_THREAD_H_ */

