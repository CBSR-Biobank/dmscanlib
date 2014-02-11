/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 *  Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE

#include "imgscanner/ImgScannerTwain.h"

#define NOMINMAX
#include <windows.h>
#include <twain.h> 

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

/**
 * Some of this code was copied from:
 *
 * https://today.java.net/pub/a/today/2004/11/18/twain.html
 */

/**
 * g_hinstDLL holds this DLL's instance handle. It is initialized in response
 * to the DLL_PROCESS_ATTACH message. This handle is passed to CreateWindow()
 * when a window is created, just before opening the data source manager.
 */
static HINSTANCE g_hinstDLL;

/**
 * g_hLib holds the handle of the TWAIN_32.DLL library. It is initialized in
 * response to the DLL_PROCESS_ATTACH message. This handle is used to obtain
 * the DSM_Entry() function address in the DLL and (if not 0) to free the DLL
 * library in response to a DLL_PROCESS_DETACH message.
 */
static HMODULE g_hLib;

/**
 * g_pDSM_Entry holds the address of function DSM_Entry() in TWAIN_32.DLL. If
 * this address is 0, either TWAIN_32.DLL could not be loaded or there is no
 * DSM_Entry() function in TWAIN_32.DLL.
 */
static DSMENTRYPROC g_pDSM_Entry;

const char * TWAIN_DLL_FILENAME = "TWAIN_32.DLL";

/**
 * g_AppID serves as a TWAIN identity structure that uniquely identifies the
 * application process responsible for making calls to function DSM_Entry().
 */
static TW_IDENTITY g_AppID = { 
	0, 
	{ 1, 0, TWLG_ENGLISH_USA, TWCY_USA, "dmscanlib 1.0" }, 
	TWON_PROTOCOLMAJOR, TWON_PROTOCOLMINOR, DG_CONTROL | DG_IMAGE, 
	"Canadian Biosample Repository",
	"Image acquisition library", 
	"dmscanlib", 
};

/**
 * DllMain
 *
 * DLL entry point. Four messages are sent from Windows to this function:
 *
 * 1) DLL_PROCESS_ATTACH
 *
 * This message is sent when the DLL is first mapped into a process's address
 * space.
 *
 * 2) DLL_THREAD_ATTACH
 *
 * This message is sent when a thread is created in the DLL's owner process.
 *
 * 3) DLL_THREAD_DETACH
 *
 * This message is sent when a thread created in the DLL's owner process is
 * terminated in any fashion other than by calling TerminateThread().
 *
 * 4) DLL_PROCESS_DETACH
 *
 * This message is sent when the DLL is unmapped from a process's address
 * space -- unless some thread calls TerminateProcess().
 *
 * Arguments:
 *
 * hinstDLL    - DLL instance handle
 * fdwReason   - reason for calling DllMain()
 * lpvReserved - additional data that may be present during DLL_PROCESS_ATTACH
 *               and DLL_PROCESS_DETACH messages
 *
 * Return:
 *
 * TRUE if DLL's initialization (during DLL_PROCESS_ATTACH) was successful, or
 * FALSE if otherwise. This return value is only recognized in response to the
 * DLL_PROCESS_ATTACH message.
 */
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
   if (fdwReason == DLL_PROCESS_ATTACH) {
       // Save instance handle for later access in other functions.
       g_hinstDLL = hinstDLL;
       g_hLib = LoadLibraryA(TWAIN_DLL_FILENAME);

       // Report failure if TWAIN_32.DLL cannot be loaded and terminate
       if (g_hLib == 0) {
           MessageBoxA(0, "Unable to open TWAIN_32.DLL", "DmScanLib", MB_OK);
           return FALSE;
       }

       // Attempt to retrieve DSM_Entry() function address.
	   g_pDSM_Entry =  (DSMENTRYPROC) GetProcAddress(g_hLib, "DSM_Entry");
       dmscanlib::imgscanner::ImgScannerTwain::setTwainDsmEntry(g_pDSM_Entry);

       // Report failure if DSM_Entry() function not found in TWAIN_32.DLL
       // and terminate 
       if (g_pDSM_Entry == 0) {
           MessageBoxA(0, "Unable to fetch DSM_Entry address", "DmScanLib", MB_OK);
           return FALSE;
       }

#ifdef _DEBUG
	   dmscanlib::DmScanLib::configLogging(2, false);
	   VLOG(1) << "DllMain: process attach";
#endif
   } else if (fdwReason == DLL_PROCESS_DETACH) {
       // If the TWAIN_32.DLL library was loaded, remove it from memory.
       if (g_hLib != 0) {
           FreeLibrary (g_hLib);
#ifdef _DEBUG
		   VLOG(1) << "DllMain: process detach";
#endif
	   }
   }

   return TRUE;
}
