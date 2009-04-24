/*******************************************************************************
 * ImageProcessor.cpp
 *
 *	Contains methods for handling the decoding of 2d barcodes from images. Mostly
 *	a functionality wrapper around libdmtx which
 ******************************************************************************/

#include "UaDebug.h"
#include "ImageProcessor.h"

#include <string.h>
#include <iostream>
using namespace std;

#pragma warning(disable : 4996)

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
      UA_ERROR(" could not create DMTX image");
   }

   UA_DOUT(1, 1, "image width: " << dmtxImageGetProp(image, DmtxPropWidth));
   UA_DOUT(1, 1, "image height: " << dmtxImageGetProp(image, DmtxPropHeight));
   UA_DOUT(1, 1, "image bits per pixel: "
           << dmtxImageGetProp(image, DmtxPropBitsPerPixel));
   UA_DOUT(1, 1, "image row size bytes: "
           << dmtxImageGetProp(image, DmtxPropRowSizeBytes));

   dec = dmtxDecodeCreate(image, 1);
   assert(dec != NULL);

   UA_DEBUG(
      // save image to a PNM file
      FILE * fh;
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
      if(msg != NULL) {
         char buf[1024];
         memcpy(buf, msg->output, msg->outputIdx);
         buf[msg->outputIdx] = 0;
         cout << "barcode: \"" << buf << "\"" << endl;
         dmtxMessageDestroy(&msg);
      }
      dmtxRegionDestroy(&reg);
   }
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

   UA_ASSERT(filename != NULL);

   dib = dibAllocate();
   UA_ASSERT(dib != NULL);
   image = createDmtxImageFromFile(filename, dib);

   if (image == NULL) {
      UA_ERROR(" could not create DMTX image");
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

   UA_ASSERT(filename != NULL);
   UA_ASSERT(dib != NULL);

   fh = fopen(filename, "r"); // C4996
   UA_ASSERT(fh != NULL);
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
