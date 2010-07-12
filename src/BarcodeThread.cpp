#include "BarcodeThread.h"
#include "Dib.h"
#include "dmtx.h"
#include "Decoder.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "BarcodeInfo.h"
#include "ProcessImageManager.h"

BarcodeThread::BarcodeThread(ProcessImageManager * manager, double scanGap,
			     unsigned squareDev,unsigned edgeThresh, unsigned corrections,
			     CvRect croppedOffset, Dib & d)
: dib(d)
{
	this->manager = manager;
	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
	this->croppedOffset = croppedOffset;

	quitMutex.lock();
	this->quitFlag = false;
	quitMutex.unlock();
}

void BarcodeThread::run()
{
	DmtxImage *image = Decoder::createDmtxImageFromDib(dib);
	DmtxDecode *dec = NULL;
	unsigned regionCount = 0;
	unsigned width, height, dpi;
	int minEdgeSize, maxEdgeSize;

	height = dib.getHeight();
	width = dib.getWidth();
	dpi = dib.getDpi();

	dec = dmtxDecodeCreate(image, 1);
	UA_ASSERT_NOT_NULL(dec);

	// slightly smaller than the new tube edge
	minEdgeSize = static_cast < unsigned >(0.08 * dpi);

	// slightly bigger than the Nunc edge
	maxEdgeSize = static_cast < unsigned >(0.18 * dpi);

	dmtxDecodeSetProp(dec, DmtxPropEdgeMin, minEdgeSize);
	dmtxDecodeSetProp(dec, DmtxPropEdgeMax, maxEdgeSize);
	dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
	dmtxDecodeSetProp(dec, DmtxPropScanGap,
			  static_cast < unsigned >(scanGap * dpi));
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, squareDev);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, edgeThresh);

	UA_DOUT(3, 7, "processImage: image width/" << width
		<< " image height/" << height
		<< " row padding/" << dmtxImageGetProp(image,
						       DmtxPropRowPadBytes)
		<< " image bits per pixel/" << dmtxImageGetProp(image,
								DmtxPropBitsPerPixel)
		<< " image row size bytes/" << dmtxImageGetProp(image,
								DmtxPropRowSizeBytes));

	while (1) {
		DmtxRegion *reg = NULL;
		BarcodeInfo *info = NULL;

		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL)
			break;

		DmtxMessage *msg =
		    dmtxDecodeMatrixRegion(dec, reg, corrections);
		if (msg != NULL) {

			info = manager->getDecoder()->addBarcodeInfo(dec, reg, msg);

			if (info != NULL) {
				if ((croppedOffset.width != 0)
						&& (croppedOffset.height != 0))
					info->alignCoordinates(croppedOffset.x,
						       	   croppedOffset.y);

				DmtxPixelLoc & tlCorner = info->getTopLeftCorner();
				DmtxPixelLoc & brCorner = info->getBotRightCorner();

				UA_DOUT(3, 8, "message "	// << *barcodeInfosIt - 1
						<< ": " << info->getMsg()
						<< " : tlCorner/" << tlCorner.
						X << "," << tlCorner.
						Y << "  brCorner/" << brCorner.
						X << "," << brCorner.Y);
			}
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);

		UA_DOUT(3, 7,
			"retrieved message from region " << regionCount++);
	}

	dmtxDecodeDestroy(&dec);
	dmtxImageDestroy(&image);

	quitMutex.lock();
	quitFlag = true;
	quitMutex.unlock();
	return;
}

bool BarcodeThread::isFinished()
{
	bool quitFlagBuf;

	quitMutex.lock();
	quitFlagBuf = quitFlag;
	quitMutex.unlock();

	return quitFlagBuf;
}

