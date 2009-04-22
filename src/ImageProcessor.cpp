/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "ImageProcessor.h"
#include "dib.h"
#include "dmtx.h"


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void decodeDib(char * filename) {
   FILE * fh;
   Dib dib;
   DmtxImage * image;
   DmtxDecode     *dec;
   DmtxRegion     *reg;
   DmtxMessage    *msg;
   int totalBytes, headerBytes;
   unsigned char *pnm;

   fh = fopen(filename, "r");
   dib = dibAllocate();
   readDibHeader(fh, dib);
   readDibPixels(fh, dib);
   fclose(fh);

//	initGrabber();
//	selectSourceAsDefault();
//	image = acquire();

   // create dmtxImage from the dib
   image = dmtxImageCreate(dibGetPixelBuffer(dib), dibGetWidth(dib), dibGetHeight(dib),
                           DmtxPack24bppRGB);
   if (image == NULL) {
      printf("ERROR: could not create DMTX image\n");
      exit(0);
   }

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

#if 0                                           \
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
