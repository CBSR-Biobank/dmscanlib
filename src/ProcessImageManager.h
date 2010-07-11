#ifndef PROCESSIMAGEMANAGER_H_
#define PROCESSIMAGEMANAGER_H_

#include <vector>
#include "cv.h"

using namespace std;

class Dib;
class BarcodeInfo;
class BarcodeThread;

class ProcessImageManager {
 public:
	ProcessImageManager(double scanGap, unsigned squareDev,
			    unsigned edgeThresh, unsigned corrections);

	void generateBarcodes(Dib * dib, vector < CvRect > & blobVector,
			vector<BarcodeInfo *> & barcodeInfos);

 private:
	static const unsigned THREAD_NUM = 8;
	static const unsigned JOIN_TIMEOUT_SEC = 10;
	static const unsigned THRESHOLD_JOIN = 1;

	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;

	void threadHandler(vector<BarcodeThread *> & threads, unsigned threshold);

};

#endif
