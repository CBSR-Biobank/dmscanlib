#ifndef PROCESSIMAGEMANAGER_H_
#define PROCESSIMAGEMANAGER_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

#include <time.h>
#include <iostream>
#include <math.h>
#include <string>
#include <limits>
#include <vector>
#include <cmath>

#include "dmtx.h"
#include "cv.h"
#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"


using namespace std;

#define THREAD_NUM 8
#define JOIN_TIMEOUT_SEC 10
#define THRESHOLD_JOIN 1

class BarcodeThread : public OpenThreads::Thread{
	
private:
	OpenThreads::Mutex quitMutex;
	volatile bool quitFlag;

	vector<BarcodeInfo *> tempBarcodeInfo;

	Dib * dib;
	CvRect croppedOffset;
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;

public:
	BarcodeThread(double scanGap,
				  unsigned squareDev,
				  unsigned edgeThresh,
				  unsigned corrections,
				  CvRect croppedOffset,
				  Dib * dib);

	virtual ~BarcodeThread(){};

	virtual void run();
	void clean();

	bool isFinished();
	vector<BarcodeInfo *> * getBarcodes();
	
};


class processImageManager{

private:
	Dib * dib;
	vector<CvRect> * blobVector;
	vector<BarcodeInfo *> * barcodeInfos;
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;

	void threadHandler(std::vector<BarcodeThread *> & threads, unsigned threshold);
	
public:
	processImageManager(Dib * dib,
						vector<CvRect> * blobVector,
						vector<BarcodeInfo *> * barcodeInfos,
						double scanGap,
						unsigned squareDev,
						unsigned edgeThresh,
						unsigned corrections);

	// populates barcodeInfos 
	void generateBarcodes(); 

};


#endif