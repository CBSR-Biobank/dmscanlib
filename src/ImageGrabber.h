#ifndef __INCLUDE_IMAGE_GRABBER_H
#define __INCLUDE_IMAGE_GRABBER_H

#ifndef WIN32
#error ERROR: should not be compiled for non-windows build
#endif

#include "dib.h"
#include "dmtx.h"
#include "twain.h"     // Standard TWAIN header.

#include <windows.h>

class ImageGrabber {
public:
	ImageGrabber();
	~ImageGrabber();

	HANDLE acquireImage();
	DmtxImage* acquireDmtxImage();
	void selectSourceAsDefault();
	void freeImage(HANDLE hanlde);
	void unloadTwain();

private:
	int GetPaletteSize(BITMAPINFOHEADER& bmInfo);
	BOOL setCapability(TW_UINT16 cap,TW_UINT16 value,BOOL sign);

	// properties used by the scanner
	const int dpi = 300;
	const int scan_CONTRAST = 500;
	const int scan_BRIGHTNESS = 500;

	// g_hinstDLL holds this DLL's instance handle. It is initialized in response
	// to the DLL_PROCESS_ATTACH message. This handle is passed to CreateWindow()
	// when a window is created, just before opening the data source manager.
	//static HINSTANCE g_hinstDLL;

	// g_hLib holds the handle of the TWAIN_32.DLL library. It is initialized in
	// response to the DLL_PROCESS_ATTACH message. This handle is used to obtain
	// the DSM_Entry() function address in the DLL and (if not 0) to free the DLL
	// library in response to a DLL_PROCESS_DETACH message.
	static HMODULE g_hLib;

	// g_pDSM_Entry holds the address of function DSM_Entry() in TWAIN_32.DLL. If
	// this address is 0, either TWAIN_32.DLL could not be loaded or there is no
	// DSM_Entry() function in TWAIN_32.DLL.
	static DSMENTRYPROC g_pDSM_Entry;

	// g_AppID serves as a TWAIN identity structure that uniquely identifies the
	// application process responsible for making calls to function DSM_Entry().
	static TW_IDENTITY g_AppID;

	// srcID serves as a TWAIN identity structure that uniquely identifies the
	// source being used
	TW_IDENTITY srcID;
};

#endif /* __INCLUDE_IMAGE_GRABBER_H */
