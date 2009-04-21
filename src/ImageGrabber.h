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
int GetPaletteSize(BITMAPINFOHEADER& bmInfo);