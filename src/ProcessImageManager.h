#ifndef PROCESSIMAGEMANAGER_H_
#define PROCESSIMAGEMANAGER_H_

#include <vector>
#include "cv.h"

using namespace std;

#define THREAD_NUM 8
#define JOIN_TIMEOUT_SEC 10
#define THRESHOLD_JOIN 1

class Dib;
class BarcodeInfo;
class BarcodeThread;

class ProcessImageManager {
 public:
	ProcessImageManager(double scanGap, unsigned squareDev,
			    unsigned edgeThresh, unsigned corrections);

	void generateBarcodes(Dib * dib, vector < CvRect > *blobVector,
			vector<BarcodeInfo *> & barcodeInfos);

 private:
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;

	void threadHandler(vector < BarcodeInfo * > & barcodeInfos,
			vector<BarcodeThread *> & threads, unsigned threshold);

};

#endif
