/*
  Dmscanlib is a software library and standalone application that scans
  and decodes libdmtx compatible test-tubes. It is currently designed
  to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
  Copyright (C) 2010 Canadian Biosample Repository

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * Implements the ImgScanner.
 *
 * This class performs all interfacing with the TWAIN driver to acquire images
 * from the scanner.
 *
 * Some portions of the implementation borrowed from EZTWAIN.
 */

#include "DmScanLibInternal.h"
#include "ImgScannerImpl.h"

#include <glog/logging.h>
#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

// Initialize g_AppID. This structure is passed to DSM_Entry() in each
// function call.
TW_IDENTITY ImgScannerImpl::g_AppID = { 0, { 1, 0, TWLG_ENGLISH_USA, TWCY_USA,
                                           "dmscanlib 1.0" }, TWON_PROTOCOLMAJOR, TWON_PROTOCOLMINOR, DG_CONTROL
                                      | DG_IMAGE, "Canadian Biosample Repository",
                                      "Image acquisition library", "dmscanlib", };

const char * ImgScannerImpl::TWAIN_DLL_FILENAME = "TWAIN_32.DLL";

/*	initGrabber() should be called prior to calling any other associated functionality,
 *	as libraries such as Twain_32.dll need to be loaded before acquire or
 *	selectDefaultAsSource work.
 */
ImgScannerImpl::ImgScannerImpl() :
      g_hLib(NULL), g_pDSM_Entry(NULL) {
   g_hLib = LoadLibraryA(TWAIN_DLL_FILENAME);

   if (g_hLib != NULL) {
      g_pDSM_Entry = (DSMENTRYPROC) GetProcAddress(g_hLib, "DSM_Entry");

      CHECK_NOTNULL(g_pDSM_Entry); // Unable to fetch DSM_Entry address
   }
}

ImgScannerImpl::~ImgScannerImpl() {
   unloadTwain();
}

bool ImgScannerImpl::twainAvailable() {
   return (g_hLib != NULL);
}

unsigned ImgScannerImpl::invokeTwain(TW_IDENTITY * srcId, unsigned long dg,
                                   unsigned dat, unsigned msg, void * ptr) {
   CHECK_NOTNULL(g_pDSM_Entry);
   unsigned r = g_pDSM_Entry(&g_AppID, srcId, dg, dat, msg, ptr);
   VLOG(5) << "invokeTwain: srcId/\""
           << ((srcId != NULL) ? srcId->ProductName : "NULL")
           << "\" dg/" << dg << " dat/" << dat << " msg/" << msg
           << " ptr/" << ptr << " returnCode/" << r;

   if ((srcId == NULL) && (r != TWRC_SUCCESS) && (r != TWRC_CHECKSTATUS)) {
      VLOG(3) << "ImgScannerImpl::invokeTwain: unsuccessful call to twain";
   }
   return r;
}
/*
 *	selectSourceAsDefault()
 *	@params - none
 *	@return - none
 *
 *	Select the source to use as default for Twain, so the source does not
 *	have to be specified every time.
 */
bool ImgScannerImpl::selectSourceAsDefault() {
   CHECK_NOTNULL(g_hLib);

   // Create a static window whose handle is passed to DSM_Entry() when we
   // open the data source manager.
   HWND hwnd = CreateWindowA("STATIC", "", WS_POPUPWINDOW, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0,
                             0 /* g_hinstDLL */, 0);

   CHECK_NOTNULL(hwnd); // Unable to create private window 

   TW_UINT16 rc;
   TW_IDENTITY srcID;

   // Open the data source manager.
   rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM,
                    (TW_MEMREF) &hwnd);

   if (rc != TWRC_SUCCESS) {
      return false;
   }

   // Display the "Select Source" dialog box for selecting a data source.
   ZeroMemory (&srcID, sizeof(srcID));
   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &srcID);

   CHECK_NE(rc, TWRC_FAILURE) << "Unable to display user interface ";
   if (rc == TWRC_CANCEL) {
      return false;
   }

   // Close the data source manager.
   invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
   DestroyWindow(hwnd);

   VLOG(3) << "selectSourceAsDefault: " << srcID.ProductName;
   return true;
}

/*
 * Opens the default data source.
 */
bool ImgScannerImpl::scannerSourceInit(HWND & hwnd, TW_IDENTITY & srcID) {

   TW_UINT16 rc;

   hwnd = CreateWindowA("STATIC", "", WS_POPUPWINDOW, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP,
			0, 0 /* g_hinstDLL */, 0);

   ShowWindow(hwnd, SW_HIDE);

   CHECK(hwnd != 0) << "Unable to create private window";

   SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

   // Open the data source manager.
   rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &hwnd);
   CHECK_EQ(rc, TWRC_SUCCESS) << "Unable to open data source manager";

   // get the default source
   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &srcID);
   if (rc != TWRC_SUCCESS) {
      if (!selectSourceAsDefault()) {
         return false;
      }
   }

   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &srcID);
   if (rc != TWRC_SUCCESS) {
      // Unable to open default data source
      invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
      VLOG(3) << "DG_CONTROL / DAT_PARENT / MSG_CLOSEDSM";
      return false;
   }
   return true;
}

void ImgScannerImpl::scannerSourceDeinit(HWND & hwnd, TW_IDENTITY & srcID) {
   // Close the data source.
   invokeTwain(&srcID, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &srcID);
   VLOG(3) << "DG_CONTROL / DAT_IDENTITY / MSG_CLOSEDS";

   // Close the data source manager.
   invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
   VLOG(3) << "DG_CONTROL / DAT_PARENT / MSG_CLOSEDSM";

   // Destroy window.
   DestroyWindow(hwnd);
}

void ImgScannerImpl::setFloatToIntPair(const double f, short & whole,
                                     unsigned short & frac) {
   double round = (f > 0) ? 0.5 : -0.5;
   const unsigned tmp = static_cast<unsigned> (f * 65536.0 + round);
   whole = static_cast<short> (tmp >> 16);
   frac = static_cast<unsigned short> (tmp & 0xffff);
}

/*
 *	@params - none
 *	@return - Image acquired from twain source, in dmtxImage format
 *
 *	Grab an image from the twain source and convert it to the dmtxImage format
 */
HANDLE ImgScannerImpl::acquireImage(unsigned dpi, int brightness, int contrast,
                                  double left, double top, double right, double bottom) {
   CHECK_NOTNULL(g_hLib);

   TW_UINT16 rc;
   TW_UINT32 handle = 0;
   TW_IDENTITY srcID;
   TW_FIX32 value;
   HWND hwnd;

   CHECK_EQ(sizeof(TW_FIX32), sizeof(long));

   if (!scannerSourceInit(hwnd, srcID)) {
      errorCode = SC_FAIL;
      return NULL;
   }

   int scannerCapability = getScannerCapabilityInternal(srcID);

   if (!(scannerCapability & CAP_IS_SCANNER)) {
      errorCode = SC_FAIL;
      return NULL;
   }
   if (!(((scannerCapability & CAP_DPI_300) && dpi == 300)
         || ((scannerCapability & CAP_DPI_400) && dpi == 400)
         || ((scannerCapability & CAP_DPI_600) && dpi == 600))) {
      errorCode = SC_INVALID_DPI;
      return NULL;
   }

	double physicalWidth = getPhysicalDimensions(srcID, ICAP_PHYSICALWIDTH);
	double physicalHeight = getPhysicalDimensions(srcID, ICAP_PHYSICALHEIGHT);

	if ((left > physicalWidth) || (top > physicalHeight)
		|| (left + right > physicalWidth) || (top + bottom > physicalHeight)) {
      errorCode = SC_INVALID_VALUE;
      return NULL;
	}

   errorCode = SC_SUCCESS;

   value.Whole = dpi;
   value.Frac = 0;
   setCapOneValue(&srcID, ICAP_XRESOLUTION, TWTY_FIX32,
                  *(unsigned long*) &value);
   setCapOneValue(&srcID, ICAP_YRESOLUTION, TWTY_FIX32,
                  *(unsigned long*) &value);

   setCapOneValue(&srcID, ICAP_PIXELTYPE, TWTY_UINT16, TWPT_RGB);
   //SetCapOneValue(&srcID, ICAP_BITDEPTH, TWTY_UINT16, 8);

   value.Whole = brightness;
   setCapOneValue(&srcID, ICAP_BRIGHTNESS, TWTY_FIX32,
                  *(unsigned long*) &value);

   value.Whole = contrast;
   setCapOneValue(&srcID, ICAP_CONTRAST, TWTY_FIX32, *(unsigned long*) &value);

   VLOG(3) << "acquireImage: source/\"" << srcID.ProductName << "\""
           << " brightness/" << brightness
           << " constrast/" << contrast
           << " left/" << left
           << " top/" << top
           << " right/" << right
           << " bottom/" << bottom;

   setCapOneValue(&srcID, ICAP_UNITS, TWTY_UINT16, TWUN_INCHES);
   TW_IMAGELAYOUT layout;
   setFloatToIntPair(left, layout.Frame.Left.Whole, layout.Frame.Left.Frac);
   setFloatToIntPair(top, layout.Frame.Top.Whole, layout.Frame.Top.Frac);
   setFloatToIntPair(right, layout.Frame.Right.Whole, layout.Frame.Right.Frac);
   setFloatToIntPair(bottom, layout.Frame.Bottom.Whole,
                     layout.Frame.Bottom.Frac);
   layout.DocumentNumber = 1;
   layout.PageNumber = 1;
   layout.FrameNumber = 1;
   rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET, &layout);

   //Prepare to enable the default data source
   TW_USERINTERFACE ui;
   ui.ShowUI = FALSE;
   ui.ModalUI = FALSE;
   ui.hParent = hwnd;

   // Enable the default data source.
   rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &ui);
   VLOG(3) << "DG_CONTROL / DAT_USERINTERFACE / MSG_ENABLEDS";
   CHECK_EQ(rc, TWRC_SUCCESS) << "Unable to enable default data source";

   if (rc == TWRC_FAILURE) {
      errorCode = SC_FAIL;
      scannerSourceDeinit(hwnd, srcID);
      VLOG(3) << "TWRC_FAILURE";
      return NULL;
   }

   MSG msg;
   TW_EVENT event;
   TW_PENDINGXFERS pxfers;

   while (GetMessage((LPMSG) &msg, 0, 0, 0)) {
      event.pEvent = (TW_MEMREF) &msg;
      event.TWMessage = MSG_NULL;

      rc = invokeTwain(&srcID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT,
                       &event);
      if (rc == TWRC_NOTDSEVENT) {
         TranslateMessage((LPMSG) &msg);
         DispatchMessage((LPMSG) &msg);
         continue;
      }

      if (event.TWMessage == MSG_CLOSEDSREQ) {
         rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE,
                          MSG_DISABLEDS, &ui);
         VLOG(3) << "got MSG_CLOSEDSREQ: sending DG_CONTROL / DAT_USERINTERFACE / MSG_DISABLEDS";
         break;
      }

      if (event.TWMessage == MSG_XFERREADY) {
         TW_IMAGEINFO ii;

         rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, &ii);
         VLOG(3) << "DG_IMAGE / DAT_IMAGEINFO / MSG_GET";

         if (rc == TWRC_FAILURE) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET,
                        &pxfers);
            VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            LOG(WARNING) << "Unable to obtain image information";
            break;
         }

         // If image is compressed or is not 8-bit color and not 24-bit
         // color ...
         if ((rc != TWRC_CANCEL) && ((ii.Compression != TWCP_NONE)
                                     || ((ii.BitsPerPixel != 8) && (ii.BitsPerPixel != 24)))) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET,
                        &pxfers);
            LOG(WARNING) << "Image compressed or not 8-bit/24-bit ";
            break;
         }

         //debug info
         VLOG(3) << "acquire:"
                 << " XResolution/" << ii.XResolution.Whole << "." << ii.XResolution.Frac
                 << " YResolution/" << ii.YResolution.Whole << "." << ii.YResolution.Frac
                 << " imageWidth/" << ii.ImageWidth
                 << " imageLength/" << ii.ImageLength
                 << " SamplesPerPixel/" << ii.SamplesPerPixel
                 << " bits per pixel/" << ii.BitsPerPixel
                 << " compression/" << ii.Compression
                 << " pixelType/" << ii.PixelType;

         // Perform the transfer.
         rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET,
                          &handle);
         VLOG(3) << "DG_IMAGE / DAT_IMAGENATIVEXFER / MSG_GET";

         // If image not successfully transferred ...
         if (rc != TWRC_XFERDONE) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET,
                        &pxfers);
            VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            LOG(WARNING) << "User aborted transfer or failure";
            errorCode = SC_INVALID_IMAGE;
            handle = 0;
            break;
         }

         // acknowledge end of transfer.
         rc = invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER,
                          &pxfers);
         VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_ENDXFER";

         if (rc == TWRC_SUCCESS) {
            if (pxfers.Count != 0) {
               // Cancel all remaining transfers.
               invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS,
                           MSG_RESET, &pxfers);
               VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            } else {
               rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE,
                                MSG_DISABLEDS, &ui);
               VLOG(3) << "DG_CONTROL / DAT_USERINTERFACE / MSG_DISABLEDS";
               break;
            }
         }
      }
   }

   scannerSourceDeinit(hwnd, srcID);
   return (HANDLE) handle;
}

HANDLE ImgScannerImpl::acquireFlatbed(unsigned dpi, int brightness, int contrast) {
	TW_IDENTITY srcID;
	HWND hwnd;

	errorCode = SC_FAIL;

	if (!scannerSourceInit(hwnd, srcID)) {
		return 0;
	}

	double physicalWidth = getPhysicalDimensions(srcID, ICAP_PHYSICALWIDTH);
	double physicalHeight = getPhysicalDimensions(srcID, ICAP_PHYSICALHEIGHT);

   scannerSourceDeinit(hwnd, srcID);

	return acquireImage(dpi, brightness, contrast, 0, 0, physicalWidth, physicalHeight);
}

BOOL ImgScannerImpl::setCapOneValue(TW_IDENTITY * srcId, unsigned Cap,
                                  unsigned ItemType, unsigned long ItemVal) {
   BOOL ret_value = FALSE;
   TW_CAPABILITY cap;
   pTW_ONEVALUE pv;

   cap.Cap = Cap; // capability id
   cap.ConType = TWON_ONEVALUE; // container type
   cap.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
   CHECK_NOTNULL(cap.hContainer);

   pv = (pTW_ONEVALUE) GlobalLock(cap.hContainer);
   CHECK_NOTNULL(pv);

   pv->ItemType = ItemType;
   pv->Item = ItemVal;
   GlobalUnlock(cap.hContainer);
   ret_value = invokeTwain(srcId, DG_CONTROL, DAT_CAPABILITY, MSG_SET, &cap);
   GlobalFree(cap.hContainer);
   return ret_value;
}

bool ImgScannerImpl::getCapability(TW_IDENTITY * srcId, TW_CAPABILITY & twCap) {
   TW_UINT16 rc;

   rc = invokeTwain(srcId, DG_CONTROL, DAT_CAPABILITY, MSG_GET, &twCap);
   return (rc == TWRC_SUCCESS);
}

/* Assuming x-y resolution are the same*/
int ImgScannerImpl::getScannerCapability() {
   TW_IDENTITY srcID;
   HWND hwnd;

   if (!scannerSourceInit(hwnd, srcID)) {
      return 0;
   }

   int capabilityCode = getScannerCapabilityInternal(srcID);
   scannerSourceDeinit(hwnd, srcID);
   VLOG(5) << "Capability code: " << capabilityCode;

   return capabilityCode;
}

int ImgScannerImpl::getScannerCapabilityInternal(TW_IDENTITY & srcID) {
   int capabilityCode = 0, xresolution = 0, yresolution = 0,
	   physicalwidth = 0, physicalheight = 0;

   setCapOneValue(&srcID, ICAP_UNITS, TWTY_UINT16, TWUN_INCHES);
   xresolution = getResolutionCapability(srcID, ICAP_XRESOLUTION);
   yresolution = getResolutionCapability(srcID, ICAP_YRESOLUTION);

   if (xresolution != yresolution) {
      return 0;
   }

   capabilityCode = xresolution;

   VLOG(5) << "Polling driver for driver type";

   if (srcID.ProductName != NULL) {
      char buf[256];

      unsigned n = strlen(srcID.ProductName);
      for (unsigned i = 0; i < n; ++i) {
         buf[i] = tolower(srcID.ProductName[i]);
      }
      buf[n] = 0;

      string productnameStr = buf;

      if (productnameStr.find("wia") != string::npos) {
         VLOG(7) << "Driver type is WIA: ProductName/" << productnameStr;
         capabilityCode |= CAP_IS_WIA;
      } else {
         VLOG(7) << "Driver type is TWAIN (default)";
      }
   }

   //-------------------------Scanner Selected----------------------------//

   //TODO CAP_IS_SCANNER: use twain specific calls to determine if a source is selected

   if ((capabilityCode & CAP_DPI_300) || (capabilityCode & CAP_DPI_400)
       || (capabilityCode & CAP_DPI_600)) {
      capabilityCode |= CAP_IS_SCANNER;
   }
   return capabilityCode;
}

int ImgScannerImpl::getResolutionCapability(TW_IDENTITY & srcID, TW_UINT16 cap) {
   pTW_RANGE pvalRange;
   bool result;
   int capabilityCode = 0;
   TW_CAPABILITY twCap;

   twCap.Cap = cap;
   twCap.ConType = TWON_DONTCARE16;
   twCap.hContainer = NULL;
   result = getCapability(&srcID, twCap);

   VLOG(5) << "Polling scanner capbility";
   if (!result) {
      LOG(WARNING) << "Failed to obtain valid dpi values";
      return 0;
   }

   VLOG(5) << "twCap.ConType = " << twCap.ConType;

   switch (twCap.ConType) {

      case TWON_RANGE: {
         double minDpi, maxDpi, stepDpi;

         VLOG(5) << "ConType = Range";

         pvalRange = (pTW_RANGE) GlobalLock(twCap.hContainer);
         if (pvalRange->ItemType == TWTY_FIX32) {
            minDpi = uint32ToFloat(pvalRange->MinValue);
            maxDpi = uint32ToFloat(pvalRange->MaxValue);
            stepDpi = uint32ToFloat(pvalRange->StepSize);

            CHECK_GT(stepDpi, 0) << "TWON_RANGE stepSize was was not greater than zero.";
            CHECK_GT(minDpi, 0) << "TWON_RANGE minDpi was was not greater than zero.";
            CHECK_GE(maxDpi, minDpi) << "TWON_RANGE minDpi > naxDpi";
            VLOG(7) << "Supports DPI Range {" << " Min:" << minDpi << " Max:" << maxDpi << " Step:" << stepDpi << " }";

            if (300 - minDpi >= 0 && 300 <= maxDpi && (int) (300 - minDpi)
                % (int) stepDpi == 0)
               capabilityCode |= CAP_DPI_300;

            if (400 - minDpi >= 0 && 400 <= maxDpi && (int) (400 - minDpi)
                % (int) stepDpi == 0)
               capabilityCode |= CAP_DPI_400;

            if (600 - minDpi >= 0 && 600 <= maxDpi && (int) (600 - minDpi)
                % (int) stepDpi == 0)
               capabilityCode |= CAP_DPI_600;
         }
         break;
      }

      case TWON_ENUMERATION: {
         VLOG(5) << "ConType = Enumeration";

         pTW_ENUMERATION pvalEnum;
         TW_UINT16 index;
         unsigned int tempDpi = 0;

         pvalEnum = (pTW_ENUMERATION) GlobalLock(twCap.hContainer);
         CHECK_NOTNULL(pvalEnum);

         VLOG(7) << "Number of supported Dpi: " << pvalEnum->NumItems;
         VLOG(7) << "Dpi ItemType: " << pvalEnum->ItemType;

		 for (index = 0; index < pvalEnum->NumItems; index++) {
			 CHECK_EQ(pvalEnum->ItemType, TWTY_FIX32)
				 << "invalid item type: " << pvalEnum->ItemType;

			 tempDpi
				 = (unsigned int) twfix32ToFloat(
				 *(TW_FIX32 *) (void *) (&pvalEnum->ItemList[index
				 * 4]));
			 VLOG(7) << "Supports DPI (f32bit): " << tempDpi;

            if (tempDpi == 300)
               capabilityCode |= CAP_DPI_300;

            if (tempDpi == 400)
               capabilityCode |= CAP_DPI_400;

            if (tempDpi == 600)
               capabilityCode |= CAP_DPI_600;
         }
         break;
      }

		 //XXX Untested
	  case TWON_ONEVALUE: {
		  VLOG(5) << "ConType = OneValue";

		  pTW_ONEVALUE pvalOneValue;
		  unsigned int tempDpi = 0;

		  pvalOneValue = (pTW_ONEVALUE) GlobalLock(twCap.hContainer);

		  CHECK_EQ(pvalOneValue->ItemType, TWTY_FIX32) << 
			  "invalid item type: " << pvalOneValue->ItemType;

		  tempDpi = (unsigned int) twfix32ToFloat(
			  *(TW_FIX32 *) (void *) (&pvalOneValue->Item));
		  VLOG(6) << "Supports DPI (f32bit): " << tempDpi;

         if (tempDpi == 300)
            capabilityCode |= CAP_DPI_300;

         if (tempDpi == 400)
            capabilityCode |= CAP_DPI_400;

         if (tempDpi == 600)
            capabilityCode |= CAP_DPI_600;
         break;
      }

      default:
         LOG(WARNING) << "Unexpected dpi contype";
         break;
   }
   GlobalUnlock(twCap.hContainer);
   GlobalFree(twCap.hContainer);
   return capabilityCode;
}

double ImgScannerImpl::getPhysicalDimensions(TW_IDENTITY & srcID, TW_UINT16 cap) {
   bool result;
   TW_CAPABILITY twCap;

   twCap.Cap = cap;
   twCap.ConType = TWON_DONTCARE16;
   twCap.hContainer = NULL;

   result = getCapability(&srcID, twCap);
   CHECK(result != NULL) << "Failed to obtain valid dpi values";

   VLOG(5) << "twCap.ConType = " << twCap.ConType;

   CHECK_EQ(twCap.ConType,  TWON_ONEVALUE)
	   << "invalid con type: " << twCap.ConType;

   pTW_ONEVALUE pvalOneValue;
   double dimension = 0;

   pvalOneValue = (pTW_ONEVALUE) GlobalLock(twCap.hContainer);

   CHECK_EQ(pvalOneValue->ItemType, TWTY_FIX32) << 
	   "invalid item type: " << pvalOneValue->ItemType;

   dimension = twfix32ToFloat(*(TW_FIX32 *) (void *) (&pvalOneValue->Item));

   GlobalUnlock(twCap.hContainer);
   GlobalFree(twCap.hContainer);
   VLOG(5) << "physical dimention " << cap << " is " << dimension;
   return dimension;
}

void ImgScannerImpl::getCustomDsData(TW_IDENTITY * srcId) {
   TW_UINT16 rc;
   TW_CUSTOMDSDATA cdata;

   rc = invokeTwain(srcId, DG_CONTROL, DAT_CUSTOMDSDATA, MSG_GET, &cdata);
   if (rc == TWRC_SUCCESS) {
      //char * o = (char *)GlobalLock(cdata.hData);
      GlobalUnlock(cdata.hData);
      GlobalFree(cdata.hData);
   }
}

inline double ImgScannerImpl::uint32ToFloat(TW_UINT32 uint32) {
   TW_FIX32 fix32 = *((pTW_FIX32) (void *) (&uint32));
   return twfix32ToFloat(fix32);
}

inline double ImgScannerImpl::twfix32ToFloat(TW_FIX32 fix32) {
   return static_cast<double> (fix32.Whole) + static_cast<double> (fix32.Frac)
      / 65536.0;
}

/*
 *	freeHandle()
 *	@params - none
 *	@return - none
 *
 *	Unlock the handle to the image from twain, and free the memory.
 */
void ImgScannerImpl::freeImage(HANDLE handle) {
   CHECK_NOTNULL(handle);
   CHECK_NOTNULL(g_hLib);
   GlobalUnlock(handle);
   GlobalFree(handle);
}

/*
 *	unloadTwain()
 *	@params - none
 *	@return - none
 *
 *	If twain_32.dll was loaded, it will be removed from memory
 */
void ImgScannerImpl::unloadTwain() {
   CHECK_NOTNULL(g_hLib);
   FreeLibrary(g_hLib);
   g_hLib = NULL;
}
