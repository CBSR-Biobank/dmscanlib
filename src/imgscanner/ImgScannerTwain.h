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
#include "Image.h"
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
		class ImgScannerTwain : public ImgScanner {
		public:
			ImgScannerTwain();
			~ImgScannerTwain();

			static void setTwainDsmEntry(DSMENTRYPROC twainDsmEntry);

			/**
			* returns false if user pressed cancel when presented with dialog box to
			* select a scanner.
			*/
			bool selectSourceAsDefault();

			int getScannerCapability();

			HANDLE acquireImage(
				const unsigned dpi,
				const int brightness,
				const int contrast,
				const cv::Rect_<float> & bbox);

			HANDLE acquireFlatbed(
				const unsigned dpi,
				const int brightness,
				const int contrast);

			void freeImage(HANDLE handle);

			int getErrorCode() { return errorCode; }

		private:

			unsigned invokeTwain(TW_IDENTITY * srcId, unsigned long dg, unsigned dat,
				unsigned msg, void * data);

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

			/**
			* g_pDSM_Entry holds the address of function DSM_Entry() in TWAIN_32.DLL. If
			* this address is 0, either TWAIN_32.DLL could not be loaded or there is no
			* DSM_Entry() function in TWAIN_32.DLL.
			*/
			static DSMENTRYPROC twainDsmEntry;

			/**
			* g_AppID serves as a TWAIN identity structure that uniquely identifies the
			* application process responsible for making calls to function DSM_Entry().
			*/
			static TW_IDENTITY twainAppID;

			int brightness;
			int contrast;
			int errorCode;
		};

	} /* namespace */

} /* namespace */

#endif /* defined (WIN32) && ! defined(__MINGW32__) */

#endif /* __INCLUDE_IMG_SCANNER_IMPL_H */
