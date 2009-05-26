#include "ImageGrabber.h"
#include "TwainException.h"
#include "UaDebug.h"

using namespace std;

// Initialise g_AppID. This structure is passed to DSM_Entry() in each
// function call.
TW_IDENTITY ImageGrabber::g_AppID = {
	0,
	{ 1, 0, TWLG_ENGLISH_USA, TWCY_USA, "ImageGrabber 1.0" },
	TWON_PROTOCOLMAJOR,
	TWON_PROTOCOLMINOR,
	DG_CONTROL | DG_IMAGE,
	"Canadian Biosample Repository",
	"Image acquisition library",
	"ImageGrabber",
};

HMODULE ImageGrabber::g_hLib = {
	LoadLibrary("TWAIN_32.DLL")
};

TW_IDENTITY ImageGrabber::srcID;

// Attempt to retrieve DSM_Entry() function address.
DSMENTRYPROC ImageGrabber::g_pDSM_Entry = {
	(DSMENTRYPROC) GetProcAddress(g_hLib, "DSM_Entry")
};

/*	initGrabber() should be called prior to calling any other associated functionality,
 *	as libraries such as Twain_32.dll need to be loaded before acquire or
 *	selectDefaultAsSource work.
 */
ImageGrabber::ImageGrabber() {
	UA_ASSERTS(ImageGrabber::g_hLib != 0,
		"ImageGrabber: Unable to open TWAIN_32.DLL");

	UA_ASSERTS(g_pDSM_Entry != 0,
		"ImageGrabber: Unable to fetch DSM_Entry address");
}

ImageGrabber::~ImageGrabber() {

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
bool ImageGrabber::selectSourceAsDefault() {
	// Create a static window whose handle is passed to DSM_Entry() when we
	// open the data source manager.
	HWND hwnd = CreateWindow ("STATIC",	"",	WS_POPUPWINDOW,	CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0,
			0 /* g_hinstDLL */, 0);

	UA_ASSERTS(hwnd != 0, "Unable to create private window ");

	TW_UINT16 rc;
	TW_IDENTITY srcID;

	// Open the data source manager.
	rc = (*g_pDSM_Entry) (&g_AppID,	0, DG_CONTROL, DAT_PARENT, MSG_OPENDSM,
			(TW_MEMREF) &hwnd);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open data source manager ");

	// Display the "Select Source" dialog box for selecting a data source.
	ZeroMemory (&srcID, sizeof(srcID));
	rc = (*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT,
			(TW_MEMREF) &srcID);

	UA_ASSERTS(rc != TWRC_FAILURE, "Unable to display user interface ");
	if (rc == TWRC_CANCEL) {
		// User pressed cancel for scanner selection
		return false;
	}

	// Close the data source manager.
	(*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM,
			(TW_MEMREF) &hwnd);
	DestroyWindow (hwnd);
	return true;
}

/*
 *	@params - none
 *	@return - Image acquired from twain source, in dmtxImage format
 *
 *	Grab an image from the twain source and convert it to the dmtxImage format
 */
HANDLE ImageGrabber::acquireImage(){
	TW_UINT32 handle = 0;

	HWND hwnd = CreateWindow ("STATIC", "",	WS_POPUPWINDOW, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP,
			0, 0 /* g_hinstDLL */, 0);

	UA_ASSERTS(hwnd != 0, "Unable to create private window");

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

	TW_UINT16 rc;

	// Open the data source manager.
	rc = (*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_PARENT, MSG_OPENDSM,
			(TW_MEMREF) &hwnd);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open data source manager");

	rc = (*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS,
			&srcID);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open default data source");

	//Prepare to enable the default data source
	TW_USERINTERFACE ui;
	ui.ShowUI = false;
	ui.ModalUI = false;
	ui.hParent = hwnd;
	// Enable the default data source.
	rc = (*g_pDSM_Entry) (&g_AppID,	&srcID,	DG_CONTROL,	DAT_USERINTERFACE,
			MSG_ENABLEDS, &ui);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to enable default data source");

	MSG msg;
	TW_EVENT event;
	TW_PENDINGXFERS pxfers;

	while (GetMessage ((LPMSG) &msg, 0, 0, 0)) {
		event.pEvent = (TW_MEMREF) &msg;
		event.TWMessage = MSG_NULL;

		rc = (*g_pDSM_Entry) (&g_AppID, &srcID, DG_CONTROL,	DAT_EVENT,
				MSG_PROCESSEVENT, (TW_MEMREF) &event);

		if (rc == TWRC_NOTDSEVENT) {
			TranslateMessage ((LPMSG) &msg);
			DispatchMessage ((LPMSG) &msg);
			continue;
		}

		if (event.TWMessage == MSG_CLOSEDSREQ)
			break;

		if (event.TWMessage == MSG_XFERREADY) {
			TW_IMAGEINFO ii;
			/*		TODO: these are the properties Adam set, should do something
				with them.
			 */
			setCapability(ICAP_UNITS, TWUN_INCHES, FALSE);
			setCapability(ICAP_XRESOLUTION, dpi, FALSE);
			setCapability(ICAP_YRESOLUTION, dpi, FALSE);
			setCapability(ICAP_PIXELTYPE, TWPT_RGB, FALSE);
			setCapability(ICAP_BITDEPTH, 8, FALSE);
			setCapability(ICAP_CONTRAST, scan_CONTRAST, FALSE);
			setCapability(ICAP_BRIGHTNESS, scan_BRIGHTNESS, FALSE);

			rc = (*g_pDSM_Entry) (&g_AppID, &srcID, DG_IMAGE, DAT_IMAGEINFO,
					MSG_GET, (TW_MEMREF) &ii);

			if (rc == TWRC_FAILURE) {
				(*g_pDSM_Entry) (&g_AppID, &srcID, DG_CONTROL, DAT_PENDINGXFERS,
						MSG_RESET, (TW_MEMREF) &pxfers);
				throw TwainException("Unable to obtain image information");
				break;
			}

			// If image is compressed or is not 8-bit color and not 24-bit
			// color ...
			if ((ii.Compression != TWCP_NONE) ||
					((ii.BitsPerPixel != 8) && (ii.BitsPerPixel != 24))) {
				(*g_pDSM_Entry) (&g_AppID, &srcID, DG_CONTROL, DAT_PENDINGXFERS,
						MSG_RESET, (TW_MEMREF) &pxfers);
				UA_ERROR("Image compressed or not 8-bit/24-bit ");
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
			rc = (*g_pDSM_Entry) (&g_AppID, &srcID, DG_IMAGE, DAT_IMAGENATIVEXFER,
					MSG_GET, (TW_MEMREF) &handle);

			// If image not successfully transferred ...
			if (rc != TWRC_XFERDONE) {
				(*g_pDSM_Entry) (&g_AppID, &srcID, DG_CONTROL, DAT_PENDINGXFERS,
						MSG_RESET, (TW_MEMREF) &pxfers);
				UA_ERROR("User aborted transfer or failure");
				break;
			}

			// Cancel all remaining transfers.
			(*g_pDSM_Entry) (&g_AppID, &srcID, DG_CONTROL, DAT_PENDINGXFERS,
					MSG_RESET, (TW_MEMREF) &pxfers);
			rc = TWRC_SUCCESS;

			break;
		}
	}

	// Close the data source.
	(*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_IDENTITY,	MSG_CLOSEDS,
			&srcID);

	// Close the data source manager.
	(*g_pDSM_Entry) (&g_AppID, 0, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM,
			(TW_MEMREF) &hwnd);

	// Destroy window.
	DestroyWindow (hwnd);
	return (HANDLE) handle;
}

DmtxImage* ImageGrabber::acquireDmtxImage(){
	HANDLE h = acquireImage();
	if (h == NULL) {
		UA_WARN("aquire returned NULL");
		return NULL;
	}
	UCHAR *lpVoid,*pBits;
	LPBITMAPINFOHEADER pHead;
	lpVoid = (UCHAR *)GlobalLock(h);
	pHead = (LPBITMAPINFOHEADER )lpVoid;
	int width = pHead->biWidth;
	int height = pHead->biHeight;
	int m_nBits = pHead->biBitCount;
	DmtxImage *theImage;

	pBits = lpVoid + sizeof(BITMAPINFOHEADER);
	theImage = dmtxImageCreate((unsigned char*)pBits, width, height, DmtxPack24bppRGB);

	int rowPadBytes = (width * m_nBits) & 0x3;

	UA_DOUT(2, 1,"createDmtxImage: " << endl
		<< "lpVoid: " << *((unsigned*) lpVoid) << endl
		<< "sizeof(BITMAPINFOHEADER): " << sizeof(BITMAPINFOHEADER) << endl
		<< "Width: " << width << endl
		<< "height: " << height << endl
		<< "towPadBytes: " << rowPadBytes);
	dmtxImageSetProp(theImage, DmtxPropRowPadBytes, rowPadBytes);
	dmtxImageSetProp(theImage, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return theImage;
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
void ImageGrabber::freeImage(HANDLE handle){
	GlobalUnlock(handle);
	GlobalFree(handle);
}
