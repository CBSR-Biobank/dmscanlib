/**
 * Implements the ImageGrabber singleton.
 *
 * This class performs all interfacing with the TWAIN driver to acquire images
 * from the scanner.
 */


#include "ImageGrabber.h"
#include "UaDebug.h"

using namespace std;

// Initialize g_AppID. This structure is passed to DSM_Entry() in each
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

const char * ImageGrabberImpl::TWAIN_DLL_FILENAME = "TWAIN_32.DLL";

const char * Decoder::INI_SECTION_NAME = "plate";

/*	initGrabber() should be called prior to calling any other associated functionality,
 *	as libraries such as Twain_32.dll need to be loaded before acquire or
 *	selectDefaultAsSource work.
 */
ImageGrabberImpl::ImageGrabberImpl() : g_hLib(NULL), g_pDSM_Entry(NULL) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(2, "ImageGrabberImpl"));
	g_hLib = LoadLibrary(TWAIN_DLL_FILENAME);

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

	if ((srcId == NULL) && (r != TWRC_SUCCESS) && (r != TWRC_CHECKSTATUS)) {
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
bool ImageGrabberImpl::selectSourceAsDefault(const char ** err) {
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

	if (rc != TWRC_SUCCESS) {
		*err = "no scanners connected.";
		return false;
	}

	// Display the "Select Source" dialog box for selecting a data source.
	ZeroMemory (&srcID, sizeof(srcID));
	rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &srcID);

	UA_ASSERTS(rc != TWRC_FAILURE, "Unable to display user interface ");
	if (rc == TWRC_CANCEL) {
		*err = "user pressed cancel for scanner selection.";
		return false;
	}

	// Close the data source manager.
	invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
	DestroyWindow (hwnd);

	UA_DOUT(2, 3, "selectSourceAsDefault: " << srcID.ProductName);
	return true;
}

void ImageGrabberImpl::setFloatToIntPair(const float f, short & whole,
		unsigned short & frac) {
	const unsigned tmp = static_cast<unsigned>(f * 65536.0 + 0.5);
	whole = static_cast<short>(tmp >> 16);
	frac  = static_cast<unsigned short>(tmp & 0xffff);
}

/*
 *	@params - none
 *	@return - Image acquired from twain source, in dmtxImage format
 *
 *	Grab an image from the twain source and convert it to the dmtxImage format
 */
HANDLE ImageGrabberImpl::acquireImage(const char ** err, double top, double left,
		double bottom, double right) {
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
		if (!selectSourceAsDefault(err)) {
			return NULL;
		}
	}

	rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &srcID);
	UA_ASSERTS(rc == TWRC_SUCCESS, "Unable to open default data source");

	UA_DOUT(2, 3, "acquireImage: source/\"" << srcID.ProductName << "\"");

	setCapability(ICAP_UNITS, TWUN_INCHES, FALSE);
	TW_IMAGELAYOUT layout;
	setFloatToIntPair(top,   layout.Frame.Top.Whole,    layout.Frame.Top.Frac);
	setFloatToIntPair(left,  layout.Frame.Left.Whole,   layout.Frame.Left.Frac);
	setFloatToIntPair(right, layout.Frame.Bottom.Whole, layout.Frame.Bottom.Frac);
	setFloatToIntPair(left,  layout.Frame.Right.Whole,  layout.Frame.Right.Frac);
	layout.DocumentNumber     = 1;
	layout.PageNumber         = 1;
	layout.FrameNumber        = 1;
	rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET, &layout);

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
			setCapability(ICAP_XRESOLUTION, DPI, FALSE);
			setCapability(ICAP_YRESOLUTION, DPI, FALSE);
			setCapability(ICAP_PIXELTYPE, TWPT_RGB, FALSE);
			setCapability(ICAP_BITDEPTH, 8, FALSE);
			setCapability(ICAP_CONTRAST, SCAN_CONTRAST, FALSE);
			setCapability(ICAP_BRIGHTNESS, SCAN_BRIGHTNESS, FALSE);

			rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, &ii);

			if (rc == TWRC_FAILURE) {
				invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
				UA_ERROR("Unable to obtain image information");
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

DmtxImage* ImageGrabberImpl::acquireDmtxImage(const char ** err){
	UA_ASSERT_NOT_NULL(g_hLib);

	HANDLE h = acquireImage(err, 0, 0, 0, 0);
	if (h == NULL) {
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
BOOL ImageGrabberImpl::setCapability(TW_UINT16 cap,TW_UINT16 value, BOOL sign) {
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

void ImageGrabberImpl::getConfigFromIni(CSimpleIniA & ini) {
	const CSimpleIniA::TKeyVal * values = ini.GetSection(INI_SECTION_NAME);
	if (values == NULL) {
		cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "] not defined in ini file." << endl
			 << "Please run calibration first." << endl;
		exit(1);
	}
	if (values->size() == 0) {
		cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "] does not define any regions." << endl
		     << "Please run calibration again." << endl;
		exit(1);
	}

	for(CSimpleIniA::TKeyVal::const_iterator it = values->begin();
		it != values->end(); it++) {
		string key(it->first.pItem);
		string value(it->second);

		region = new DecodeRegion;
		UA_ASSERT_NOT_NULL(region);

		pos =  key.find(label);
		if (pos == string::npos) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		pos = key.find_first_of('_');
		if (pos == string::npos) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		string numStr = key.substr(labelSize, pos - labelSize);
		if (!Util::strToNum(numStr, region->row, 10)) {
			cerr << "INI file error: section " << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		numStr = key.substr(pos + 1);
		if (!Util::strToNum(numStr, region->col, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		pos = value.find_first_of(',');
		numStr = value.substr(0, pos);
		if (!Util::strToNum(numStr, region->topLeft.X, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], first value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->topLeft.Y, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], second value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->botRight.X, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], third value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		numStr = value.substr(pos + 1);
		if (!Util::strToNum(numStr, region->botRight.Y, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], fourth value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		decodeRegions.push_back(region);
		UA_DOUT(1, 3, "getRegionsFromIni: " << *region);
	}
}

