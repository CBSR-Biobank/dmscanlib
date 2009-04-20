// jtwain.cpp
#include "ImageGrabber.h"
#include "TwainException.h"

// The following macros serve a useful purpose: in the event of failure, they
// allow the executing thread to skip past a region of code that should not be
// executed because of that failure, while allowing that thread to perform
// necessary cleanup prior to leaving the current function. For example, a
// thread has created a window, opened the source manager connected to that
// window, and is attempting to open a source. But for some reason, the source
// can't be opened. The thread must back out of the function, first closing
// the source manager and then destroying the window. The thread must not
// execute any other code. The macros make that happen.

// BLOCK_BEGIN(x) and BLOCK_END(x) surround a block of code where a failure
// might occur, such as code that attempts to open a source. The x is a
// placeholder for a number that makes it relatively easy to distinguish
// between nested blocks, but serves no other purpose. Those two macros are
// implemented as a do { ... } while (0); construct. That construct executes
// the code between do and while exactly once. If a thread needs to
// prematurely exit from that block, it calls the EXIT_CURRENT_BLOCK macro, a
// break statement.

#define BLOCK_BEGIN(x)      do {
#define BLOCK_END(x)        } while (0);
#define EXIT_CURRENT_BLOCK  break;

// g_hinstDLL holds this DLL's instance handle. It is initialized in response
// to the DLL_PROCESS_ATTACH message. This handle is passed to CreateWindow()
// when a window is created, just before opening the data source manager.

static HINSTANCE g_hinstDLL;

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

// Function prototype for JTwainException exception-throwing helper function.

//static void throwJTE (JNIEnv *env, const char *msg);

// Function prototypes for image transfer helper functions.
int GetPaletteSize(BITMAPINFOHEADER& bmInfo);
static DmtxImage* createDmtxImage  (HANDLE hMem);
//static jobject xferDIB24toImage (LPBITMAPINFOHEADER lpbmih, JNIEnv *env);

// ===========================================================================
// DllMain
//
// DLL entry point. Four messages are sent from Windows to this function:
//
// 1) DLL_PROCESS_ATTACH
//
// This message is sent when the DLL is first mapped into a process's address
// space.
//
// 2) DLL_THREAD_ATTACH
//
// This message is sent when a thread is created in the DLL's owner process.
//
// 3) DLL_THREAD_DETACH
//
// This message is sent when a thread created in the DLL's owner process is
// terminated in any fashion other than by calling TerminateThread().
//
// 4) DLL_PROCESS_DETACH
//
// This message is sent when the DLL is unmapped from a process's address
// space -- unless some thread calls TerminateProcess().
//
// Arguments:
//
// hinstDLL    - DLL instance handle
// fdwReason   - reason for calling DllMain()
// lpvReserved - additional data that may be present during DLL_PROCESS_ATTACH
//               and DLL_PROCESS_DETACH messages
//
// Return:
//
// TRUE if DLL's initialization (during DLL_PROCESS_ATTACH) was successful, or
// FALSE if otherwise. This return value is only recognized in response to the
// DLL_PROCESS_ATTACH message.
// ===========================================================================


//BOOL initialize(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
BOOL initGrabber()
{
 //  if (fdwReason == DLL_PROCESS_ATTACH)
 //  {
       // Save instance handle for later access in other functions.

 //      g_hinstDLL = hinstDLL;

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
   /* }
   else
   if (fdwReason == DLL_PROCESS_DETACH)
   {
       // If the TWAIN_32.DLL library was loaded, remove it from memory.

       if (g_hLib != 0)
           FreeLibrary (g_hLib);
   }*/

   return TRUE;
}

// ===========================================================================
// acquire
// ===========================================================================
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

   BLOCK_BEGIN(1)
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
       EXIT_CURRENT_BLOCK
   }

   BLOCK_BEGIN(2)

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
       EXIT_CURRENT_BLOCK
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
       EXIT_CURRENT_BLOCK
   }

   BLOCK_BEGIN(3)

   if (rc != TWRC_SUCCESS)
   {
       throw TwainException("Unable to enable default data source (acquire)");
       EXIT_CURRENT_BLOCK
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

          // Perform the transfer.
          TW_UINT32 handle;
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
          image = createDmtxImage ((HANDLE)handle);

          // If unable to transfer image, throw an exception upon return to
          // Java.

/*          if (image == 0)
              throw TwainException("Could not transfer DIB to Image (acquire)");
*/
          GlobalUnlock ((HANDLE) handle);
          GlobalFree ((HANDLE) handle);

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
   BLOCK_END(3)

   // Close the data source.
   (*g_pDSM_Entry) (&g_AppID,
                    0,
                    DG_CONTROL,
                    DAT_IDENTITY,
                    MSG_CLOSEDS,
                    &srcID);
   BLOCK_END(2)

   // Close the data source manager.
   (*g_pDSM_Entry) (&g_AppID,
                    0,
                    DG_CONTROL,
                    DAT_PARENT,
                    MSG_CLOSEDSM,
                    (TW_MEMREF) &hwnd);
   BLOCK_END(1)

   // Destroy window.
   DestroyWindow (hwnd);
   //return (rc == TWRC_SUCCESS) ? image : (jobject) 0;
   return image;
}

// ===========================================================================
// selectSourceAsDefault
// ===========================================================================
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

   BLOCK_BEGIN(1)
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
       EXIT_CURRENT_BLOCK
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
   BLOCK_END(1)

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

	
	pBits = lpVoid + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*GetPaletteSize(*pHead);
	theImage = dmtxImageCreate((unsigned char*)pBits, width, height, DmtxPack32bppRGBX);

	int bytesPerpixel = m_nBits >> 3;
	int rowPadBytes = (width * m_nBits) & 0x3;

	dmtxImageSetProp(theImage, DmtxPropRowPadBytes, rowPadBytes);
    dmtxImageSetProp(theImage, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return theImage;
}

int GetPaletteSize(BITMAPINFOHEADER& bmInfo)
{
	switch(bmInfo.biBitCount)
	{
		case 1:		
			return 2;
		case 4:		
			return 16;
		case 8:		
			return 256;
		default:	
			return 0;
	}
}