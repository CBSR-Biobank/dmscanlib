// jtwain.cpp
#include "ImageGrabber.h"
#include "TwainException.h"

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

//handle to the image to be unlocked and freed later.
TW_UINT32 handle;

/*
*	initGrabber()
*	@params - none
*	@return - TW_BOOL corresponding to success of loading the appropriate libraries
*
*	initGrabber() should be called prior to calling any other associated functionality,
*	as libraries such as Twain_32.dll need to be loaded before acquire or 
*	selectDefaultAsSource work.
*/
TW_BOOL initGrabber()
{
	// Create a buffer for holding the Windows directory path.
	char szPath [150]; // Probably only 140 is needed, but why not be safe?

	// Retrieve the path of the Windows directory.
	GetWindowsDirectory (szPath, 128);

	// Obtain number of characters copied into the buffer.
	int iLen = lstrlen (szPath);

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
	if (OpenFile (szPath, &ofs, OF_EXIST) != -1)
		g_hLib = LoadLibrary (szPath);

	// Report failure if TWAIN_32.DLL cannot be loaded and terminate the
	// JTWAIN DLL.
	if (g_hLib == 0)
	{
		MessageBox (0, "Unable to open TWAIN_32.DLL", "ImageGrabber", MB_OK);
		return FALSE;
	}

	// Attempt to retrieve DSM_Entry() function address.
	g_pDSM_Entry = (DSMENTRYPROC) GetProcAddress (g_hLib, "DSM_Entry");

	// Report failure if DSM_Entry() function not found in TWAIN_32.DLL
	// and terminate the JTWAIN DLL.
	if (g_pDSM_Entry == 0)
	{
		MessageBox (0, "Unable to fetch DSM_Entry address", "ImageGrabber", 
			MB_OK);
		return FALSE;
	}

	// Initialize g_AppID. This structure is passed to DSM_Entry() in each
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

	lstrcpy (g_AppID.Manufacturer, "Matt Radkie");
	lstrcpy (g_AppID.ProductFamily, "Image acquisition library");
	lstrcpy (g_AppID.ProductName, "ImageGrabber");

	return TRUE;
}

/*
* acuire()
*	@params - none
*	@return - Image acquired from twain source, in dmtxImage format
*
*	Grab an image from the twain source and convert it to the dmtxImage format
*/
DmtxImage* acquire(){
	DmtxImage *image;

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
		throw TwainException("Unable to create private window (acquire)");
		//return (jobject) 0;
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
	TW_IDENTITY srcID;
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
//		SetCapability(ICAP_UNITS, TWUN_INCHES, FALSE);
//		SetCapability(ICAP_XRESOLUTION, dpi, FALSE);
//		SetCapability(ICAP_YRESOLUTION, dpi, FALSE);
/*		SetCapability(ICAP_PIXELTYPE, TWPT_RGB, FALSE);
		rc = (g_pDSM_Entry) (&g_AppID,
					&srcID,
					DG_CONTROL,
					DAT_CAPABILITY,
					MSG_SET,
					(TW_MEMREF)&cap);
		SetCapability(ICAP_PIXELTYPE, TWPT_BW, FALSE);
		SetCapability(ICAP_BITDEPTH, 8, FALSE);*/
//		SetCapability(ICAP_CONTRAST, scan_CONTRAST, FALSE);
//		SetCapability(ICAP_BRIGHTNESS, scan_BRIGHTNESS, FALSE);
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
#ifdef _DEBUG
	std::cout << "================================ acquire =========================\n";
	std::cout << "Bits per pixel: " << ii.BitsPerPixel << "\n";
	std::cout << "Compression: " << ii.Compression << "\n";
	std::cout << "ImageLength: " << ii.ImageLength << "\n";
	std::cout << "ImageWidth: " << ii.ImageWidth << "\n";
	std::cout << "PixelType: " << ii.PixelType << "\n";
#endif		

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
			//TODO: check if 8 or 24 bit and handle accordingly
			image = createDmtxImage ((HANDLE)handle);

			//GlobalUnlock ((HANDLE) handle);
			//GlobalFree ((HANDLE) handle);

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
	return image;
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
void selectSourceAsDefault()
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

/*
*	 ===========================================================================
*	 Create a dmtx image from the handle to the twain image
*	 ===========================================================================
*
*	@param - handle to the twain image
*	@return - original scanned image stored as a dmtximage
*
*	As of the 0.70 release of libdmtx, images are stored as a 1D array of unsigned
*	char, so conversion from the windows bitmap format to dmtx image should be
*	straight forward.
*/
DmtxImage* createDmtxImage(HANDLE hMem)
{
	UCHAR *lpVoid,*pBits;
	LPBITMAPINFOHEADER pHead;
	lpVoid = (UCHAR *)GlobalLock(hMem);
	pHead = (LPBITMAPINFOHEADER )lpVoid;
	int width = pHead->biWidth;
	int height = pHead->biHeight;
	int m_nBits = pHead->biBitCount;
	DmtxImage *theImage;


	pBits = lpVoid + sizeof(BITMAPINFOHEADER);
	theImage = dmtxImageCreate((unsigned char*)pBits, width, height, DmtxPack24bppRGB);

	int bytesPerpixel = m_nBits >> 3;
	int rowPadBytes = (width * m_nBits) & 0x3;

#ifdef _DEBUG
	std::cout << "====================== createDmtxImage ==============================\n";
	std::cout << "lpVoid: " << *((unsigned*) lpVoid) << "\n";
	std::cout << "sizeof(BITMAPINFOHEADER): " << sizeof(BITMAPINFOHEADER) << "\n";
	std::cout << "Width: " << width << "\n";
	std::cout << "height: " << height << "\n";
	std::cout << "towPadBytes: " << rowPadBytes << "\n";
#endif

	dmtxImageSetProp(theImage, DmtxPropRowPadBytes, rowPadBytes);
	dmtxImageSetProp(theImage, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return theImage;
}

/*
*	unloadTwain()
*	@params - none
*	@return - none
*
*	If twain_32.dll was loaded, it will be removed from memory
*/
void unloadTwain(){
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
void freeHandle(){
	GlobalUnlock ((HANDLE) handle);
	GlobalFree ((HANDLE) handle);
}