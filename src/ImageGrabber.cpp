#include "ImageGrabber.h"
#include "TwainException.h"
#include "UaDebug.h"

using namespace std;

/*	initGrabber() should be called prior to calling any other associated functionality,
 *	as libraries such as Twain_32.dll need to be loaded before acquire or
 *	selectDefaultAsSource work.
 */
ImageGrabber::ImageGrabber() {
	// Create a buffer for holding the Windows directory path.
	char szPath [150]; // Probably only 140 is needed, but why not be safe?

	// Retrieve the path of the Windows directory.
	GetWindowsDirectory(szPath, 128);

	// Obtain number of characters copied into the buffer.
	int iLen = lstrlen(szPath);

	// Path ends in a backslash character if the directory is the root.
	// Otherwise, path does not end in backslash. In that case, we must
	// append a backslash character to the path.
	if (iLen != 0 && szPath [iLen-1] != '\\')
		lstrcat (szPath, "\\");

	// Append TWAIN_32.DLL to the path. This is the 32-bit TWAIN DLL that
	// we need to communicate with.
	lstrcat (szPath, "TWAIN_32.DLL");

	// If the TWAIN_32.DLL file exists in the path (which is determined by
	// opening and closing that file), attempt to load TWAIN_32.DLL into
	// the calling process's address space.
	OFSTRUCT ofs;
	if (OpenFile(szPath, &ofs, OF_EXIST) != -1)
		g_hLib = LoadLibrary(szPath);

	// Report failure if TWAIN_32.DLL cannot be loaded and terminate the
	// JTWAIN DLL.
	if (g_hLib == 0) {
		UA_ERROR("ImageGrabber: Unable to open TWAIN_32.DLL");
	}

	// Attempt to retrieve DSM_Entry() function address.
	g_pDSM_Entry = (DSMENTRYPROC) GetProcAddress (g_hLib, "DSM_Entry");

	// Report failure if DSM_Entry() function not found in TWAIN_32.DLL
	// and terminate the JTWAIN DLL.
	if (g_pDSM_Entry == 0) {
		UA_ERROR("ImageGrabber: Unable to fetch DSM_Entry address");
	}

	// Initialise g_AppID. This structure is passed to DSM_Entry() in each
	// function call.
	g_AppID.Id = 0;
	g_AppID.Version.MajorNum = 1;
	g_AppID.Version.MinorNum = 0;
	g_AppID.Version.Language = TWLG_ENGLISH_USA;
	g_AppID.Version.Country = TWCY_USA;

	lstrcpy (g_AppID.Version.Info, "ImageGrabber 1.0");

	g_AppID.ProtocolMajor = TWON_PROTOCOLMAJOR;
	g_AppID.ProtocolMinor = TWON_PROTOCOLMINOR;
	g_AppID.SupportedGroups = DG_CONTROL | DG_IMAGE;

	lstrcpy (g_AppID.Manufacturer, "Canadian Biosample Repository");
	lstrcpy (g_AppID.ProductFamily, "Image acquisition library");
	lstrcpy (g_AppID.ProductName, "ImageGrabber");
}

ImageGrabber::~ImageGrabber() {

}

/*
 * acuire()
 *	@params - none
 *	@return - Image acquired from twain source, in dmtxImage format
 *
 *	Grab an image from the twain source and convert it to the dmtxImage format
 */
HANDLE ImageGrabber::acquireImage(){
	TW_UINT32 handle = NULL;

	HWND hwnd = CreateWindow ("STATIC",
			"",
			WS_POPUPWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			HWND_DESKTOP,
			0,
			0,//g_hinstDLL,
			0);

	if (hwnd == 0) {
		throw TwainException("Unable to create private window (acquire)");
	}

	SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

	TW_UINT16 rc;

	// Open the data source manager.
	rc = (*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_PARENT,
			MSG_OPENDSM,
			(TW_MEMREF) &hwnd);

	if (rc != TWRC_SUCCESS)
	{
		throw TwainException("Unable to open data source manager (acquire)");
	}

	// Get the default data source's name.
	ZeroMemory (&srcID, sizeof(srcID));
	rc = (*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_GETDEFAULT,
			&srcID);

	if (rc == TWRC_FAILURE)
	{
		throw TwainException("Unable to obtain default data source name (acquire)");
	}

	rc = (*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_OPENDS,
			&srcID);

	if (rc != TWRC_SUCCESS)
	{
		throw TwainException("Unable to open default data source (acquire)");
	}

	//Prepare to enable the default data source
	TW_USERINTERFACE ui;
	ui.ShowUI = false;
	ui.ModalUI = false;
	ui.hParent = hwnd;
	// Enable the default data source.
	rc = (*g_pDSM_Entry) (&g_AppID,
			&srcID,
			DG_CONTROL,
			DAT_USERINTERFACE,
			MSG_ENABLEDS,
			&ui);

	if (rc != TWRC_SUCCESS)
	{
		throw TwainException("Unable to enable default data source (acquire)");
	}

	MSG msg;
	TW_EVENT event;
	TW_PENDINGXFERS pxfers;

	while (GetMessage ((LPMSG) &msg, 0, 0, 0))
	{
		event.pEvent = (TW_MEMREF) &msg;
		event.TWMessage = MSG_NULL;

		rc = (*g_pDSM_Entry) (&g_AppID,
				&srcID,
				DG_CONTROL,
				DAT_EVENT,
				MSG_PROCESSEVENT,
				(TW_MEMREF) &event);

		if (rc == TWRC_NOTDSEVENT)
		{
			TranslateMessage ((LPMSG) &msg);
			DispatchMessage ((LPMSG) &msg);
			continue;
		}

		if (event.TWMessage == MSG_CLOSEDSREQ)
			break;

		if (event.TWMessage == MSG_XFERREADY)
		{
			TW_IMAGEINFO ii;
			/*		TODO: these are the properties Adam set, should do something
				with them.
			 */
			SetCapability(ICAP_UNITS, TWUN_INCHES, FALSE);
			SetCapability(ICAP_XRESOLUTION, dpi, FALSE);
			SetCapability(ICAP_YRESOLUTION, dpi, FALSE);
			SetCapability(ICAP_PIXELTYPE, TWPT_RGB, FALSE);
			SetCapability(ICAP_BITDEPTH, 8, FALSE);
			SetCapability(ICAP_CONTRAST, scan_CONTRAST, FALSE);
			SetCapability(ICAP_BRIGHTNESS, scan_BRIGHTNESS, FALSE);

			rc = (*g_pDSM_Entry) (&g_AppID,
					&srcID,
					DG_IMAGE,
					DAT_IMAGEINFO,
					MSG_GET,
					(TW_MEMREF) &ii);

			if (rc == TWRC_FAILURE)
			{
				(*g_pDSM_Entry) (&g_AppID,
						&srcID,
						DG_CONTROL,
						DAT_PENDINGXFERS,
						MSG_RESET,
						(TW_MEMREF) &pxfers);
				throw TwainException("Unable to obtain image information (acquire)");
				break;
			}

			// If image is compressed or is not 8-bit color and not 24-bit
			// color ...
			if (ii.Compression != TWCP_NONE ||
					ii.BitsPerPixel != 8 &&
					ii.BitsPerPixel != 24)
			{
				(*g_pDSM_Entry) (&g_AppID,
						&srcID,
						DG_CONTROL,
						DAT_PENDINGXFERS,
						MSG_RESET,
						(TW_MEMREF) &pxfers);
				throw TwainException("Image compressed or not 8-bit/24-bit "
						"(acquire)");
				break;
			}

			//debug info
			UA_DOUT(2, 1, "ImageGrabber::acquire" << endl
					<< "Bits per pixel: " << ii.BitsPerPixel << endl
					<< "Compression: " << ii.Compression << endl
					<< "ImageLength: " << ii.ImageLength << endl
					<< "ImageWidth: " << ii.ImageWidth << endl
					<< "PixelType: " << ii.PixelType);

			// Perform the transfer.
			rc = (*g_pDSM_Entry) (&g_AppID,
					&srcID,
					DG_IMAGE,
					DAT_IMAGENATIVEXFER,
					MSG_GET,
					(TW_MEMREF) &handle);

			// If image not successfully transferred ...
			if (rc != TWRC_XFERDONE)
			{
				(*g_pDSM_Entry) (&g_AppID,
						&srcID,
						DG_CONTROL,
						DAT_PENDINGXFERS,
						MSG_RESET,
						(TW_MEMREF) &pxfers);
				throw TwainException("User aborted transfer or failure (acquire)");
				break;
			}

			// Cancel all remaining transfers.
			(*g_pDSM_Entry) (&g_AppID,
					&srcID,
					DG_CONTROL,
					DAT_PENDINGXFERS,
					MSG_RESET,
					(TW_MEMREF) &pxfers);
			rc = TWRC_SUCCESS;

			break;
		}
	}

	// Close the data source.
	(*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_CLOSEDS,
			&srcID);

	// Close the data source manager.
	(*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_PARENT,
			MSG_CLOSEDSM,
			(TW_MEMREF) &hwnd);

	// Destroy window.
	DestroyWindow (hwnd);
	//return (rc == TWRC_SUCCESS) ? image : (jobject) 0;
	return (HANDLE) handle;
}

/*
 *	selectSourceAsDefault()
 *	@params - none
 *	@return - none
 *
 *	Select the source to use as default for Twain, so the source does not
 *	have to be specified every time.
 *	TODO: change return type to void?
 */
void ImageGrabber::selectSourceAsDefault()
{
	// Create a static window whose handle is passed to DSM_Entry() when we
	// open the data source manager.
	HWND hwnd = CreateWindow ("STATIC",
			"",
			WS_POPUPWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			HWND_DESKTOP,
			0,
			0,//g_hinstDLL,
			0);
	if (hwnd == 0)
	{
		throw TwainException("Unable to create private window "
				"(selectSourceAsDefault)");
		return;
	}

	TW_UINT16 rc;
	TW_IDENTITY srcID;

	// Open the data source manager.
	rc = (*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_PARENT,
			MSG_OPENDSM,
			(TW_MEMREF) &hwnd);

	if (rc != TWRC_SUCCESS)
	{
		throw TwainException("Unable to open data source manager "
				"(selectSourceAsDefault)");
	}

	// Display the "Select Source" dialog box for selecting a data source.
	ZeroMemory (&srcID, sizeof(srcID));
	rc = (*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_USERSELECT,
			(TW_MEMREF) &srcID);
	if (rc == TWRC_FAILURE)
		throw TwainException("Unable to display user interface "
				"(selectSourceAsDefault)");

	// Close the data source manager.
	(*g_pDSM_Entry) (&g_AppID,
			0,
			DG_CONTROL,
			DAT_PARENT,
			MSG_CLOSEDSM,
			(TW_MEMREF) &hwnd);
	DestroyWindow (hwnd);
}

DmtxImage* acquireDmtxImage(){
	TW_UINT32 h = acquire();
	if (h == NULL) {
		UA_WARN("aquire returned NULL");
		return NULL;
	}

	//TODO: check if 8 or 24 bit and handle accordingly
	return createDmtxImage((HANDLE)h);
}

/*
Sets the capability of the Twain Data Source
 */
BOOL ImageGrabber::setCapability(TW_UINT16 cap,TW_UINT16 value,BOOL sign)
{

	TW_CAPABILITY twCap;
	pTW_ONEVALUE pVal;
	BOOL ret_value = FALSE;

	twCap.Cap = cap;
	twCap.ConType = TWON_ONEVALUE;

	twCap.hContainer = GlobalAlloc(GHND,sizeof(TW_ONEVALUE));
	if(twCap.hContainer)
	{
		pVal = (pTW_ONEVALUE)GlobalLock(twCap.hContainer);
		pVal->ItemType = sign ? TWTY_INT16 : TWTY_UINT16;
		pVal->Item = (TW_UINT32)value;
		GlobalUnlock(twCap.hContainer);
		// change this?
		ret_value = (*g_pDSM_Entry)(&g_AppID,&srcID,DG_CONTROL,DAT_CAPABILITY,MSG_SET,(TW_MEMREF)&twCap);
		GlobalFree(twCap.hContainer);
	}
	return ret_value;

}

/*
 *	unloadTwain()
 *	@params - none
 *	@return - none
 *
 *	If twain_32.dll was loaded, it will be removed from memory
 */
void ImageGrabber::unloadTwain(){
	if (g_hLib != 0)
		FreeLibrary (g_hLib);
}

/*
 *	freeHandle()
 *	@params - none
 *	@return - none
 *
 *	Unlock the handle to the image from twain, and free the memory.
 */
void ImageGrabber::freeHandle(HANDLE handle){
	GlobalUnlock(handle);
	GlobalFree(handle);
}
