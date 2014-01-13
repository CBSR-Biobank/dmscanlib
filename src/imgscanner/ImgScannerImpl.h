#ifndef __INCLUDE_IMG_SCANNER_IMPL_H
#define __INCLUDE_IMG_SCANNER_IMPL_H
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

#if defined (WIN32) && ! defined(__MINGW32__)

#include "imgscanner/ImgScanner.h"
#include "dib/Dib.h"
#include "DmScanLib.h"

#include <dmtx.h>
#include <twain.h>     // Standard TWAIN header.
#define NOMINMAX
#include <windows.h>

namespace dmscanlib {

    namespace imgscanner {

        /**
         * This class interfaces with the TWAIN driver to acquire images from the
         * scanner.
         */
        class ImgScannerImpl : public ImgScanner {
        public:
            ImgScannerImpl();
            ~ImgScannerImpl();

            bool twainAvailable();

            /**
             * returns false if user pressed cancel when presented with dialog box to
             * select a scanner.
             */
            bool selectSourceAsDefault();

            int getScannerCapability();

            HANDLE acquireImage(unsigned dpi, int brightness, int contrast,
                    const ScanRegion<double> & bbox);

            HANDLE acquireFlatbed(unsigned dpi, int brightness, int contrast);

            void freeImage(HANDLE handle);

            int getErrorCode() {return errorCode;}

        private:

            unsigned invokeTwain(TW_IDENTITY * srcId, unsigned long dg, unsigned dat,
                    unsigned msg, void * data);

            void unloadTwain();

            void setFloatToIntPair(const double f, short & whole, unsigned short & frac);
            int getPaletteSize(BITMAPINFOHEADER& bmInfo);

            //BOOL setCapability(TW_IDENTITY * srcId, TW_UINT16 cap,TW_UINT16 value,BOOL sign);

            BOOL setCapOneValue(TW_IDENTITY * srcId, unsigned Cap, unsigned ItemType, unsigned long ItemVal);

            bool getCapability(TW_IDENTITY * srcId, TW_CAPABILITY & twCap);

            inline double uint32ToFloat(TW_UINT32 uint32);
            inline double twfix32ToFloat(TW_FIX32 fix32);

            void getCustomDsData(TW_IDENTITY * srcId);

            bool scannerSourceInit(HWND & hwnd, TW_IDENTITY & srcID);
            void scannerSourceDeinit(HWND & hwnd, TW_IDENTITY & srcID);

            int getScannerCapabilityInternal(TW_IDENTITY & srcID);
            int getResolutionCapability(TW_IDENTITY & srcID, TW_UINT16 cap);
            double getPhysicalDimensions(TW_IDENTITY & srcID, TW_UINT16 cap);

            static const char * TWAIN_DLL_FILENAME;

            // g_hinstDLL holds this DLL's instance handle. It is initialized in response
            // to the DLL_PROCESS_ATTACH message. This handle is passed to CreateWindow()
            // when a window is created, just before opening the data source manager.
            //static HINSTANCE g_hinstDLL;

            // g_hLib holds the handle of the TWAIN_32.DLL library. It is initialized in
            // response to the DLL_PROCESS_ATTACH message. This handle is used to obtain
            // the DSM_Entry() function address in the DLL and (if not 0) to free the DLL
            // library in response to a DLL_PROCESS_DETACH message.
            HMODULE g_hLib;

            // g_pDSM_Entry holds the address of function DSM_Entry() in TWAIN_32.DLL. If
            // this address is 0, either TWAIN_32.DLL could not be loaded or there is no
            // DSM_Entry() function in TWAIN_32.DLL.
            DSMENTRYPROC g_pDSM_Entry;

            // g_AppID serves as a TWAIN identity structure that uniquely identifies the
            // application process responsible for making calls to function DSM_Entry().
            static TW_IDENTITY g_AppID;

            int brightness;
            int contrast;
            int errorCode;
        };

    } /* namespace */

} /* namespace */

#endif /* defined (WIN32) && ! defined(__MINGW32__) */

#endif /* __INCLUDE_IMG_SCANNER_IMPL_H */
