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
class ProcessImageManager;

class BarcodeThread : public OpenThreads::Thread {
public:
	BarcodeThread(ProcessImageManager * manager, double scanGap, unsigned squareDev, unsigned edgeThresh,
		      unsigned corrections, CvRect croppedOffset, Dib & dib);

	virtual ~ BarcodeThread() {
	};

	virtual void run();

	bool isFinished();

 private:
	OpenThreads::Mutex quitMutex;
	volatile bool quitFlag;

	Dib & dib;
	CvRect croppedOffset;
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;
	ProcessImageManager * manager;
};

#endif /* BARCODE_THREAD_H_ */

