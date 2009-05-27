/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaDebug.h"
#include "Dib.h"
#include "LinkList.h"

#include <string.h>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

Decoder::Decoder(Dib * dib) :
	results(new LinkList()) {
	decodeImage(dib);
}

Decoder::Decoder(DmtxImage * image) :
	results(new LinkList()) {
	decodeImage(image);
}

Decoder::~Decoder() {
	delete results;
}

/*
 *	decodeDib
 *	@params - filename: char* corresponding to the filename of an image
 *	@return - The number of bytes that got truncated to fit the barcodes
 *			  in the supplied buffer.
 *
 *	Create a file from the filename given, then create a DmtxImage from this
 *	file. If a DmxtImage can be created, decode it. All barcodes decoded are
 *	stored in the supplied buffer, up to a max length of bufferSize.
 */
void Decoder::decodeImage(Dib * dib){
	UA_ASSERT_NOT_NULL(dib);
	UA_ASSERT_NOT_NULL(results);

	DmtxImage * image = createDmtxImageFromDib(dib);
	decodeImage(image);
	dmtxImageDestroy(&image);
}

void Decoder::decodeImage(DmtxImage * image) {
	UA_ASSERT_NOT_NULL(image);

	if (results != NULL) {
		// an image was already created, destroy this one as a new one
		// is created below
		delete results;
	}
	results = new LinkList();

	DmtxDecode * dec;
	DmtxRegion * reg;
	DmtxMessage * msg;
	FILE * fh;
	unsigned char *pnm;
	int totalBytes, headerBytes;

	UA_DOUT(1, 3, "Decoder::decodeImage: image width/"
			<< dmtxImageGetProp(image, DmtxPropWidth)
			<< " image height/" << dmtxImageGetProp(image, DmtxPropHeight)
			<< " row padding/" << dmtxImageGetProp(image, DmtxPropRowPadBytes)
			<< " image bits per pixel/"
			<< dmtxImageGetProp(image, DmtxPropBitsPerPixel)
			<< " image row size bytes/"
			<< dmtxImageGetProp(image, DmtxPropRowSizeBytes));

	dec = dmtxDecodeCreate(image, 1);
	assert(dec != NULL);

	// save image to a PNM file
	UA_DEBUG(
			pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
			fh = fopen("out.pnm", "w");
			fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
			fclose(fh);
	);

	dmtxDecodeSetProp(dec, DmtxPropScanGap, DmtxPropScanGap);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, DmtxPropSquareDevn);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, DmtxPropEdgeThresh);

	reg = dmtxRegionFindNext(dec, NULL);

	if (reg != NULL) {
		UA_DOUT(1, 1, "found a region...");
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			char * buffer = new char[msg->outputIdx + 1];
			UA_ASSERT_NOT_NULL(buffer);
			memcpy(buffer, msg->output, msg->outputIdx);
			buffer[msg->outputIdx] = 0;
			UA_DOUT(1, 5, "barcode is: " << buffer);
			results->append(buffer);
			dmtxMessageDestroy(&msg);
		}
		else {
			//no message decoded, write 10 0's
			//TODO: put rotation code here?
			UA_DOUT(1, 1, "Unable to read region");
		}

		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
}

/**
 *	createDmtxImageFromFile
 *
 *	Open the file and create a DmtxImage out of it.
 *
 *	@params - filename: char* corresponding to the filename of an image
 *			  dib - a blank Divice Independant Bitmap to read the image into
 *	@return - DmtxImage: the newly created image from the file.
 *
 */
DmtxImage * Decoder::createDmtxImageFromDib(Dib * dib) {
	UA_ASSERT_NOT_NULL(dib);

	int pack = DmtxPack24bppRGB;

	if (dib->getBitsPerPixel() == 32) {
		pack = DmtxPack32bppXRGB;
	}

	// create dmtxImage from the dib
	DmtxImage * image = dmtxImageCreate(dib->getPixelBuffer(), dib->getWidth(),
			dib->getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dib->getRowPadBytes());
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return image;
}

unsigned Decoder::getNumTags() {
	UA_ASSERT_NOT_NULL(results);
	return results->size();
}

char * Decoder::getTag(int tagNum) {
	UA_ASSERT_NOT_NULL(results);
	return (char *) results->getItem(tagNum);
}

