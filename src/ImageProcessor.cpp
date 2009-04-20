/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "dib.h"
#include "dmtx.h"
#include "ImageGrabber.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char ** argv) {
   FILE * fh;
   Dib * dib, *dib2;
   DmtxImage * image;
   DmtxDecode     *dec;
   DmtxRegion     *reg;
   DmtxMessage    *msg;
   int totalBytes, headerBytes;
   unsigned char *pnm;

	if (argc != 2) {
		printf("ERROR: no input image specified\n");
		exit(0);
	}
	// create dib from file argument
	fh = fopen(argv[1], "r");
   dib = dibAllocate();
   readDibHeader(fh, dib);
   readDibPixels(fh, dib);
   fclose(fh);
	initGrabber();
	selectSourceAsDefault();
	image = acquire();
   // create dmtxImage from the dib
   //image = dmtxImageCreate(dibGetPixelBuffer(dib), dibGetWidth(dib), dibGetHeight(dib),
   //   DmtxPack24bppRGB);
   assert(image != NULL);
   //set the properties (pad bytes, flip)
   dmtxImageSetProp(image, DmtxPropRowPadBytes, dibGetRowPadBytes(dib));
   dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	//debugging info
   printf("image width: %d\n", dmtxImageGetProp(image, DmtxPropWidth));
   printf("image height: %d\n", dmtxImageGetProp(image, DmtxPropHeight));
   printf("image bits per pixel: %d\n", dmtxImageGetProp(image, DmtxPropBitsPerPixel));
   printf("image row size bytes: %d\n", dmtxImageGetProp(image, DmtxPropRowSizeBytes));

   dec = dmtxDecodeCreate(image, 1);
   assert(dec != NULL);
   
#if 0
   // save image to a PNM file
   pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
   fh = fopen("out.pnm", "w");
   fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
   fclose(fh);
   exit(0);
#endif

   dmtxDecodeSetProp(dec, DmtxPropScanGap, DmtxPropScanGap);
   dmtxDecodeSetProp(dec, DmtxPropSquareDevn, DmtxPropSquareDevn);
   dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, DmtxPropEdgeThresh);

   reg = dmtxRegionFindNext(dec, NULL);
   if (reg != NULL) {
      printf("found a region...\n");
      msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
      if(msg != NULL) {
         fputs("output: \"", stdout);
         fwrite(msg->output, sizeof(unsigned char), msg->outputIdx, stdout);
         fputs("\"\n", stdout);
         dmtxMessageDestroy(&msg);
      }
      dmtxRegionDestroy(&reg);
   }
   dmtxDecodeDestroy(&dec);
   dmtxImageDestroy(&image);

   dibDestroy(dib);
}
