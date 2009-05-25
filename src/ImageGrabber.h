#ifndef __INCLUDE_IMAGE_GRABBER_H
#define __INCLUDE_IMAGE_GRABBER_H

#ifndef _VISUALC_
#error ERROR: should not be compiled for non-windows build
#endif

#include <windows.h>

#include "dib.h"
#include "dmtx.h"
#include "twain.h"     // Standard TWAIN header.
//#include "TwainException.h"

TW_BOOL initGrabber();
DmtxImage* acquire();
void selectSourceAsDefault();
static DmtxImage* createDmtxImage(HANDLE hMem);
void unloadTwain();
void freeHandle();
int GetPaletteSize(BITMAPINFOHEADER& bmInfo);
BOOL SetCapability(TW_UINT16 cap,TW_UINT16 value,BOOL sign);

#endif /* __INCLUDE_IMAGE_GRABBER_H */
