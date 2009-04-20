#include <windows.h>

#include "dib.h"
#include "dmtx.h"
#include "twain.h"     // Standard TWAIN header.
//#include "TwainException.h"

BOOL initGrabber();
DmtxImage* acquire();
void selectSourceAsDefault();
static DmtxImage* createDmtxImage(HANDLE hMem);