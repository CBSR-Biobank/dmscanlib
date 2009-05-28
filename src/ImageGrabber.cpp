#include "ImageGrabber.h"
#include "TwainException.h"
#include "UaDebug.h"

using namespace std;

// Initialise g_AppID. This structure is passed to DSM_Entry() in each
// function call.
TW_IDENTITY ImageGrabberImpl::g_AppID = {
	0,
	{ 1, 0, TWLG_ENGLISH_USA, TWCY_USA, "scanlib 1.0" },
	TWON_PROTOCOLMAJOR,
	TWON_PROTOCOLMINOR,
	DG_CONTROL | DG_IMAGE,
	"Canadian Biosample Repository",
	"Image acquisition library",
	"scanlib",
};

/*	initGrabber() should be called prior to calling any other associated functionality,
 *	as libraries such as Twain_32.dll need to be loaded before acquire or
 *	selectDefaultAsSource work.
 */
ImageGrabberImpl::ImageGrabberImpl() : g_hLib(NULL), g_pDSM_Entry(NULL) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(2, "ImageGrabberImpl"));
	g_hLib = LoadLibrary("TWAIN_32.DLL");

	if (g_hLib != NULL) {
		g_pDSM_Entry = (DSMENTRYPROC) GetProcAddress(g_hLib, "DSM_Entry");

		UA_ASSERTS(g_pDSM_Entry != 0,
				"ImageGrabberImpl: Unable to fetch DSM_Entry address");
	}
}

ImageGrabberImpl::~ImageGrabberImpl() {
	unloadTwain();
}

bool ImageGrabberImpl::twainAvailable() {
	return (g_hLib != NULL);
}

unsigned ImageGrabberImpl::invokeTwain(TW_IDENTITY * srcId, unsigned long dg,
		unsigned dat, unsigned msg, void * ptr) {
	UA_ASSERT_NOT_NULL(g_pDSM_Entry);
	unsigned r = (*g_pDSM_Entry) (&g_AppID, srcId, dg, dat, msg, ptr);
	UA_DOUT(2, 3, "ImageGrabberImpl::invokeTwain: srcId/\""
			<< ((srcId != NULL) ? srcId->ProductName : "NULL")
			<< "\" dg/" << dg << " dat/" << dat << " msg/" << msg
			<< " ptr/" << ptr << " returnCode/" << r);

	if ((srcId == NULL) && (r != TWRC_SUCCESS)) {
		UA_DOUT(2, 3, "ImageGrabberImpl::invokeTwain: unsuccessful call to twain");
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
 *	TODO: change return type to void?
 */
bool ImageGrabberImpl::selectSourceAsDefault() {
	UA_ASSERT_NOT_NULL(g_hLib);

	// Create a static window whose handle is passed to DSM_Entry() when we
	// open the data source manager.
	HWND hwnd = CreateWindow ("STATIC",	"",	WS_POPUPWINDOW,	CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0,
			0 /* g_hinstDLL */, 0);

	UA_ASSERTS(hwnd != 0, "Unable to create private window ");

	TW_UINT16 rc;
	TW_IDENTITY srcID;

	// Open the data source manager.
	rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF) &hwnd);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open data source manager ");

	// Display the "Select Source" dialog box for selecting a data source.
	ZeroMemory (&srcID, sizeof(srcID));
	rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &srcID);

	UA_ASSERTS(rc != TWRC_FAILURE, "Unable to display user interface ");
	if (rc == TWRC_CANCEL) {
		// User pressed cancel for scanner selection
		return false;
	}

	// Close the data source manager.
	invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
	DestroyWindow (hwnd);

	UA_DOUT(2, 3, "selectSourceAsDefault: " << srcID.ProductName);
	return true;
}

/*
 *	@params - none
 *	@return - Image acquired from twain source, in dmtxImage format
 *
 *	Grab an image from the twain source and convert it to the dmtxImage format
 */
HANDLE ImageGrabberImpl::acquireImage(){
	UA_ASSERT_NOT_NULL(g_hLib);

	TW_UINT32 handle = 0;
	TW_IDENTITY srcID;

	HWND hwnd = CreateWindow ("STATIC", "",	WS_POPUPWINDOW, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP,
			0, 0 /* g_hinstDLL */, 0);

	UA_ASSERTS(hwnd != 0, "Unable to create private window");

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

	TW_UINT16 rc;

	// Open the data source manager.
	rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &hwnd);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open data source manager");

	// get the default source
	rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &srcID);
	if (rc != TWRC_SUCCESS) {
		UA_DOUT(2, 3, "acquireImage: Unable to open default data source");
		return NULL;
	}

	rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &srcID);
	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open default data source");

	UA_DOUT(2, 3, "acquireImage: source/\""
			<< srcID.ProductName << "\"");

	//Prepare to enable the default data source
	TW_USERINTERFACE ui;
	ui.ShowUI = false;
	ui.ModalUI = false;
	ui.hParent = hwnd;
	// Enable the default data source.
	rc = invokeTwain(&srcID, DG_CONTROL,	DAT_USERINTERFACE, MSG_ENABLEDS, &ui);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to enable default data source");

	MSG msg;
	TW_EVENT event;
	TW_PENDINGXFERS pxfers;

	while (GetMessage ((LPMSG) &msg, 0, 0, 0)) {
		event.pEvent = (TW_MEMREF) &msg;
		event.TWMessage = MSG_NULL;

		rc = invokeTwain(&srcID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &event);

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

			rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, &ii);

			if (rc == TWRC_FAILURE) {
				invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
				throw TwainException("Unable to obtain image information");
				break;
			}

			// If image is compressed or is not 8-bit color and not 24-bit
			// color ...
			if ((ii.Compression != TWCP_NONE) ||
					((ii.BitsPerPixel != 8) && (ii.BitsPerPixel != 24))) {
				invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
				UA_ERROR("Image compressed or not 8-bit/24-bit ");
				break;
			}

			//debug info
			UA_DOUT(2, 1, "acquire:"
					<< " Bits per pixel/" << ii.BitsPerPixel
					<< " Compression/" << ii.Compression
					<< " ImageLength/" << ii.ImageLength
					<< " ImageWidth/" << ii.ImageWidth
					<< " PixelType/" << ii.PixelType);

			// Perform the transfer.
			rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET, &handle);

			// If image not successfully transferred ...
			if (rc != TWRC_XFERDONE) {
				invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
				UA_ERROR("User aborted transfer or failure");
				break;
			}

			// Cancel all remaining transfers.
			invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
			rc = TWRC_SUCCESS;

			break;
		}
	}

	// Close the data source.
	invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY,	MSG_CLOSEDS, &srcID);

	// Close the data source manager.
	invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);

	// Destroy window.
	DestroyWindow (hwnd);
	return (HANDLE) handle;
}

DmtxImage* ImageGrabberImpl::acquireDmtxImage(){
	UA_ASSERT_NOT_NULL(g_hLib);

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

	UA_DOUT(2, 1,"acquireDmtxImage: " << endl
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
 * Sets the capability of the Twain Data Source
 */
BOOL ImageGrabberImpl::setCapability(TW_UINT16 cap,TW_UINT16 value,BOOL sign) {
	UA_ASSERT_NOT_NULL(g_hLib);

	TW_CAPABILITY twCap;
	pTW_ONEVALUE pVal;
	BOOL ret_value = FALSE;
	TW_IDENTITY srcID;

	// get the default source
	TW_UINT16 rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &srcID);

	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open default data source");

	twCap.Cap = cap;
	twCap.ConType = TWON_ONEVALUE;

	twCap.hContainer = GlobalAlloc(GHND,sizeof(TW_ONEVALUE));
	if (twCap.hContainer) {
		pVal = (pTW_ONEVALUE)GlobalLock(twCap.hContainer);
		pVal->ItemType = sign ? TWTY_INT16 : TWTY_UINT16;
		pVal->Item = (TW_UINT32)value;
		GlobalUnlock(twCap.hContainer);
		// change this?
		ret_value = invokeTwain(&srcID, DG_CONTROL, DAT_CAPABILITY, MSG_SET, &twCap);
		GlobalFree(twCap.hContainer);
	}
	return ret_value;
}

/*
 *	freeHandle()
 *	@params - none
 *	@return - none
 *
 *	Unlock the handle to the image from twain, and free the memory.
 */
void ImageGrabberImpl::freeImage(HANDLE handle){
	UA_ASSERT_NOT_NULL(g_hLib);

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
void ImageGrabberImpl::unloadTwain(){
	UA_ASSERT_NOT_NULL(g_hLib);
	FreeLibrary(g_hLib);
	g_hLib = NULL;
}
