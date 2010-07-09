#include "processImageManager.h"

processImageManager::processImageManager(double scanGap,unsigned squareDev,
										 unsigned edgeThresh,unsigned corrections){
	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
}

void processImageManager::threadHandler(vector<BarcodeInfo *> * barcodeInfos,
										vector<BarcodeThread *> & threads, 
										unsigned threshold){
	time_t timeStart,timeEnd;

	time(&timeStart);

	std::vector<BarcodeThread *> tempthreads;

	while(1) {

		for(unsigned j=0; j < threads.size(); j++){
			if(threads[j]->isFinished()){
				std::vector<BarcodeInfo *> * barcodes = threads[j]->getBarcodes();

				for(unsigned z=0; z < barcodes->size(); z++)
					barcodeInfos->push_back((*barcodes)[z]);

				threads[j]->clean();
			}
			else{
				tempthreads.push_back(threads[j]);
			}
		}
		threads = tempthreads;
		tempthreads.clear();

		if(threads.size() < threshold)
			break;
		else
			sleep(1);

		/*----join----*/
		if(threshold == THRESHOLD_JOIN){
			time(&timeEnd);
			if(difftime(timeEnd,timeStart) >= JOIN_TIMEOUT_SEC){
				UA_DOUT(3, 1, "Error:: Some threads have timed out.");
				break;
			}
		}


	}
}

void processImageManager::generateBarcodes(Dib * dib,
						vector<CvRect> * blobVector,
						vector<BarcodeInfo *> * barcodeInfos){

	UA_DOUT(3, 5, "getTubeBlobs found: " << blobVector->size() << " blobs.");

	std::vector<BarcodeThread *> threads;

	for (int i =0 ;i<(int)blobVector->size();i++){

		/*---thread controller (limit # threads to THREAD_NUM)----*/
		threadHandler(barcodeInfos,threads,THREAD_NUM);

		Dib * tmp = new Dib;
		tmp->crop(*dib,
			(*blobVector)[i].x,
			(*blobVector)[i].y,
			(*blobVector)[i].x+(*blobVector)[i].width,
			(*blobVector)[i].y+(*blobVector)[i].height);

		BarcodeThread * thread = new BarcodeThread(this->scanGap,
			this->squareDev,
			this->edgeThresh,
			this->corrections,
			(*blobVector)[i],
			tmp);

		threads.push_back(thread);
		thread->run();

	}

	/*---join---*/
	threadHandler(barcodeInfos,threads,THRESHOLD_JOIN);



	//TODO add upper bound to blob size
	std::vector<BarcodeInfo *> tempBarcodes;
	for(unsigned y=0; y < barcodeInfos->size(); y++){

		bool uniqueBarcode = true;
		for(unsigned x=0; x < tempBarcodes.size(); x++)
			if(tempBarcodes[x]->Equals((*barcodeInfos)[y])){
				uniqueBarcode = false;
				break;
			}
		
		if(uniqueBarcode)
			tempBarcodes.push_back((*barcodeInfos)[y]);
	}

	barcodeInfos->clear();
	*barcodeInfos = tempBarcodes;
}

/*-----------------------------------Barcode Thread-------------------------------------------*/

BarcodeThread::BarcodeThread(double scanGap,
							 unsigned squareDev,
							 unsigned edgeThresh,
							 unsigned corrections,
							 CvRect croppedOffset,
							 Dib * dib){
								 this->scanGap = scanGap;
								 this->squareDev = squareDev;
								 this->edgeThresh = edgeThresh;
								 this->corrections = corrections;
								 this->croppedOffset = croppedOffset;
								 this->dib = dib;

								 quitMutex.lock();
								 this->quitFlag = false;
								 quitMutex.unlock();
}


void BarcodeThread::run() {
	DmtxImage * image = createDmtxImageFromDib(*(this->dib));
	DmtxDecode * dec = NULL;
	unsigned regionCount = 0;
	unsigned width,height,dpi;
	int minEdgeSize,maxEdgeSize;

	height = this->dib->getHeight();
	width = this->dib->getWidth();
	dpi = this->dib->getDpi();

	dec = dmtxDecodeCreate(image, 1);
	UA_ASSERT_NOT_NULL(dec);

	// slightly smaller than the new tube edge
	minEdgeSize = static_cast<unsigned> (0.08 * dpi);

	// slightly bigger than the Nunc edge
	maxEdgeSize = static_cast<unsigned> (0.18 * dpi);

	dmtxDecodeSetProp(dec, DmtxPropEdgeMin, minEdgeSize);
	dmtxDecodeSetProp(dec, DmtxPropEdgeMax, maxEdgeSize);
	dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
	dmtxDecodeSetProp(dec, DmtxPropScanGap, static_cast<unsigned> (this->scanGap* dpi));
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, this->squareDev);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, this->edgeThresh);

	UA_DOUT(3, 7, "processImage: image width/" << width
		<< " image height/" << height
		<< " row padding/" << dmtxImageGetProp(image, DmtxPropRowPadBytes)
		<< " image bits per pixel/"
		<< dmtxImageGetProp(image, DmtxPropBitsPerPixel)
		<< " image row size bytes/"
		<< dmtxImageGetProp(image, DmtxPropRowSizeBytes));

	while (1) {
		DmtxRegion * reg = NULL;
		BarcodeInfo * info = NULL;

		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL)
			break;

		DmtxMessage * msg = dmtxDecodeMatrixRegion(dec, reg, this->corrections);
		if (msg != NULL) {
			info = new BarcodeInfo(dec, reg, msg);
			UA_ASSERT_NOT_NULL(info);

			if(this->croppedOffset.width !=0 && this->croppedOffset.height !=0)
				info->alignCoordinates(this->croppedOffset.x,this->croppedOffset.y);

			tempBarcodeInfo.push_back(info);

			DmtxPixelLoc & tlCorner = info->getTopLeftCorner();
			DmtxPixelLoc & brCorner = info->getBotRightCorner();

			UA_DOUT(3, 8, "message " // << *barcodeInfosIt - 1
				<< ": " << info->getMsg()
				<< " : tlCorner/" << tlCorner.X << "," << tlCorner.Y
				<< "  brCorner/" << brCorner.X << "," << brCorner.Y);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);

		UA_DOUT(3, 7, "retrieved message from region " << regionCount++);
	}

	dmtxDecodeDestroy(&dec);
	dmtxImageDestroy(&image);

	delete this->dib;
	this->dib = NULL;

	quitMutex.lock();
	this->quitFlag = true;
	quitMutex.unlock();
	return;
}

bool BarcodeThread::isFinished(){
	bool quitFlagBuf;

	quitMutex.lock();
	quitFlagBuf = this->quitFlag;
	quitMutex.unlock();

	return quitFlagBuf;
}

vector<BarcodeInfo *> * BarcodeThread::getBarcodes(){
	if(this->isFinished()){
		return &( this->tempBarcodeInfo );
	}
	else{
		return NULL;
	}
}
void BarcodeThread::clean(){
	//quitMutex.~Mutex();
}
