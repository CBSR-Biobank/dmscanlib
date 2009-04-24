/*******************************************************************************
* ImageProcessor.cpp
*
*	Contains methods for handling the decoding of 2d barcodes from images. Mostly
*	a functionality wrapper around libdmtx which 
******************************************************************************/
#include "ImageProcessor.h"

/*
*	decodeDmtxImage
*	@params - image: pointer to the DmtxImage to decode
*	@return - none
*
*	As of right now, this function takes a DmtxImage as the parameter,
*	and decodes a single 2d barcode in the Image. 
*
*	TODO: return the message instead of printing it out.
*	TODO: Improve decoding (decode more than one barcode)
*/
void decodeDmtxImage(DmtxImage* image){
	DmtxDecode     *dec;
	DmtxRegion     *reg;
	DmtxMessage    *msg;
	int totalBytes, headerBytes;
	unsigned char *pnm;
	if (image == NULL) {
		printf("ERROR: could not read DMTX image\n");
		exit(0);
	}

	dec = dmtxDecodeCreate(image, 1);
	assert(dec != NULL);
	//debugging info
#ifdef _DEBUG                                           \
	// save image to a PNM file
	FILE * fh;
	printf("======================= decodeDmtxImage ===================================\n");
	printf("image width: %d\n", dmtxImageGetProp(image, DmtxPropWidth));
	printf("image height: %d\n", dmtxImageGetProp(image, DmtxPropHeight));
	printf("image bits per pixel: %d\n", dmtxImageGetProp(image, DmtxPropBitsPerPixel));
	printf("image row size bytes: %d\n", dmtxImageGetProp(image, DmtxPropRowSizeBytes));
	pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
	fh = fopen("out.pnm", "w");
	fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
	fclose(fh);
	//exit(0);
#endif

	dmtxDecodeSetProp(dec, DmtxPropScanGap, DmtxPropScanGap);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, DmtxPropSquareDevn);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, DmtxPropEdgeThresh);

	reg = dmtxRegionFindNext(dec, NULL);
	int read = 0;
	int total = 0;
	if( reg != NULL) printf("found a region...\n");

	while (reg != NULL) {	
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if(msg != NULL) {
			fputs("output: \"", stdout);
			fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
			fputs("\"\n", stdout);
			dmtxMessageDestroy(&msg);
			read++;
		}
		else {
			//TODO: rotation calls here?
			printf("couldnt read message!\n");
		}
		dmtxRegionDestroy(&reg);
		reg = dmtxRegionFindNext(dec, NULL);
		total++;
	}
	printf("Found a total of %d regions, and successfully decoded %d of them", total, read);
	dmtxDecodeDestroy(&dec);
}

/*
*	decodeDib
*	@params - filename: char* corresponding to the filename of an image
*	@return - none
*
*	Create a file from the filename given, then create a DmtxImage from this
*	file. If a DmxtImage can be created, decode it.
*
*	TODO: return the decoded string.
*/
void decodeDib(char * filename) {
	Dib dib;
	DmtxImage *image;


	dib = dibAllocate();
	image = createDmtxImageFromFile(filename, dib);
	
	if (image == NULL) {
		printf("ERROR: could not create DMTX image\n");
		exit(0);
	}

	decodeDmtxImage(image);
	dmtxImageDestroy(&image);
	dibDestroy(dib);
}

/*
*	createDmtxImageFromFile
*	@params - filename: char* corresponding to the filename of an image
*			  dib - a blank Divice Independant Bitmap to read the image into
*	@return - DmtxImage: the newly created image from the file.
*
*	Open the file and create a DmtxImage out of it.
*/
DmtxImage * createDmtxImageFromFile(char* filename, Dib dib){
	FILE * fh;
	DmtxImage * image;

	fh = fopen(filename, "r");
	readDibHeader(fh, dib);
	readDibPixels(fh, dib);
	fclose(fh);

	// create dmtxImage from the dib
	image = dmtxImageCreate(dibGetPixelBuffer(dib), dibGetWidth(dib), dibGetHeight(dib),
		DmtxPack24bppRGB);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dibGetRowPadBytes(dib));
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y

	return image;
}