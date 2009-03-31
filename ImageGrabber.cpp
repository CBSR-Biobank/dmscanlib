// jtwain.cpp
#include <windows.h>
#include "stdafx.h"
#include "TwainImage.h"
#include "twain.h"     // Standard TWAIN header.

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

static TwainImage xferDIB8toImage  (LPBITMAPINFOHEADER lpbmih);
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


BOOL initialize(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH)
   {
       // Save instance handle for later access in other functions.

       g_hinstDLL = hinstDLL;

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
           MessageBox (0, "Unable to open TWAIN_32.DLL", "JTWAIN", MB_OK);
           return FALSE;
       }

       // Attempt to retrieve DSM_Entry() function address.

       g_pDSM_Entry = (DSMENTRYPROC) GetProcAddress (g_hLib, "DSM_Entry");

       // Report failure if DSM_Entry() function not found in TWAIN_32.DLL
       // and terminate the JTWAIN DLL.

       if (g_pDSM_Entry == 0)
       {
           MessageBox (0, "Unable to fetch DSM_Entry address", "JTWAIN", 
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

       lstrcpy (g_AppID.Version.Info, "ImageLib 1.0");

       g_AppID.ProtocolMajor = TWON_PROTOCOLMAJOR;
       g_AppID.ProtocolMinor = TWON_PROTOCOLMINOR;
       g_AppID.SupportedGroups = DG_CONTROL | DG_IMAGE;

       lstrcpy (g_AppID.Manufacturer, "Matt Radkie");
       lstrcpy (g_AppID.ProductFamily, "Image acquisition library");
       lstrcpy (g_AppID.ProductName, "ImageLib");
   }
   else
   if (fdwReason == DLL_PROCESS_DETACH)
   {
       // If the TWAIN_32.DLL library was loaded, remove it from memory.

       if (g_hLib != 0)
           FreeLibrary (g_hLib);
   }

   return TRUE;
}

// ===========================================================================
// Java_net_javajeff_jtwain_JTwain_acquire
// ===========================================================================
void acquire(){
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
                             g_hinstDLL,
                             0);

   // If window could not be created, throw exception. Because the exception
   // is not actually thrown until execution returns to Java, we must return
   // a value -- (jobject) 0 was chosen to represent Image null. This value
   // will not be seen in the Java code because of the exception.

   if (hwnd == 0)
   {
       throw TwainException("Unable to create private window (acquire)");
       //return (jobject) 0;
   }

   // Ensure that the default data source's dialog box does not disappear
   // behind other windows, which can be very disconcerting to the user. We do
   // that by making the hwnd -- created above and passed to DSM_Entry() below
   // -- the handle of the topmost window.

   SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

   TW_UINT16 rc;
   //jobject image = 0;

   BLOCK_BEGIN(1)

   // Open the data source manager.

   rc = (*g_pDSM_Entry) (&g_AppID,
                         0,
                         DG_CONTROL,
                         DAT_PARENT,
                         MSG_OPENDSM,
                         (TW_MEMREF) &hwnd);

   // If data source manager could not be opened, throw exception. Because the
   // exception is not actually thrown until execution returns to Java, we
   // first exit current block to destroy previously-created window and return
   // a value (which isn't seen in the Java code).

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

   // If failure occurred, prepare to throw an exception, which is only thrown
   // after this function ends.

   if (rc == TWRC_FAILURE)
   {
       throw TwainException("Unable to obtain default data source name (acquire)");
       EXIT_CURRENT_BLOCK
   }

   // Open the default data source.

   rc = (*g_pDSM_Entry) (&g_AppID,
                         0,
                         DG_CONTROL,
                         DAT_IDENTITY,
                         MSG_OPENDS,
                         &srcID);

   // If default data source could not be opened, throw exception. Because the
   // exception is not actually thrown until execution returns to Java, we
   // first exit current block to close data source manager and destroy the
   // previously-created window.

   if (rc != TWRC_SUCCESS)
   {
       throw TwainException("Unable to open default data source (acquire)");
       EXIT_CURRENT_BLOCK
   }

   BLOCK_BEGIN(3)

   // Prepare to enable the default data source. Make sure to show the data
   // source's own user interface (dialog box).

   TW_USERINTERFACE ui;
   ui.ShowUI = TRUE;
   ui.ModalUI = FALSE;
   ui.hParent = hwnd;

   // Enable the default data source.

   rc = (*g_pDSM_Entry) (&g_AppID,
                         &srcID,
                         DG_CONTROL,
                         DAT_USERINTERFACE,
                         MSG_ENABLEDS,
                         &ui);

   // If default data source could not be enabled, throw exception. Because
   // the exception is not actually thrown until execution returns to Java, we
   // first exit current block to close data source, close data source
   // manager and destroy the previously-created window.

   if (rc != TWRC_SUCCESS)
   {
       throw TwainException("Unable to enable default data source (acquire)");
       EXIT_CURRENT_BLOCK
   }

   // Begin the event-handling loop. Data transfer takes place in this loop.

   MSG msg;
   TW_EVENT event;
   TW_PENDINGXFERS pxfers;

   while (GetMessage ((LPMSG) &msg, 0, 0, 0))
   {
      // Each window message must be forwarded to the default data source.

      event.pEvent = (TW_MEMREF) &msg;
      event.TWMessage = MSG_NULL;

      rc = (*g_pDSM_Entry) (&g_AppID,
                            &srcID,
                            DG_CONTROL,
                            DAT_EVENT,
                            MSG_PROCESSEVENT,
                            (TW_MEMREF) &event);

      // If the message does not correspond to a data source event, we must
      // dispatch it to the appropriate Windows window.

      if (rc == TWRC_NOTDSEVENT)
      {             
          TranslateMessage ((LPMSG) &msg);
          DispatchMessage ((LPMSG) &msg);
          continue;
      }

      // If the default data source is requesting that the data source's
      // dialog box be closed (user pressed Cancel), we must break out of the
      // message loop.

      if (event.TWMessage == MSG_CLOSEDSREQ)
          break;

      // If the default data source is requesting that it is ready to begin
      // the data transfer, we must perform that transfer.

      if (event.TWMessage == MSG_XFERREADY)
      {
          // Obtain information about the first image to be transferred.

          TW_IMAGEINFO ii;
          rc = (*g_pDSM_Entry) (&g_AppID,
                                &srcID,
                                DG_IMAGE,
                                DAT_IMAGEINFO,
                                MSG_GET,
                                (TW_MEMREF) &ii);

          // If unable to obtain image information ...

          if (rc == TWRC_FAILURE)
          {
              // Cancel all transfers.

              (*g_pDSM_Entry) (&g_AppID,
                               &srcID,
                               DG_CONTROL,
                               DAT_PENDINGXFERS,
                               MSG_RESET,
                               (TW_MEMREF) &pxfers);

              // Throw exception upon return to Java and break out of event
              // loop.

              throw TwainException("Unable to obtain image information (acquire)");
              break;
          }

          // If image is compressed or is not 8-bit color and not 24-bit
          // color ...

          if (ii.Compression != TWCP_NONE ||
              ii.BitsPerPixel != 8 &&
              ii.BitsPerPixel != 24)
          {
              // Cancel all transfers.

              (*g_pDSM_Entry) (&g_AppID,
                               &srcID,
                               DG_CONTROL,
                               DAT_PENDINGXFERS,
                               MSG_RESET,
                               (TW_MEMREF) &pxfers);

              // Throw exception upon return to Java and break out of event
              // loop.

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
              // Cancel all remaining transfers.

              (*g_pDSM_Entry) (&g_AppID,
                               &srcID,
                               DG_CONTROL,
                               DAT_PENDINGXFERS,
                               MSG_RESET,
                               (TW_MEMREF) &pxfers);

              // Throw exception upon return to Java and break out of event
              // loop.

              throw TwainException("User aborted transfer or failure (acquire)");
              break;
          }

          // Transfer Windows-based DIB to a Java-based Image (via a
          // MemoryImageSource).

          LPBITMAPINFOHEADER lpbmih;
          lpbmih = (LPBITMAPINFOHEADER) GlobalLock ((HANDLE) handle);
/*  TODO
          if (ii.BitsPerPixel == 8)
              image = xferDIB8toImage (lpbmih, env);
          else
              image = xferDIB24toImage (lpbmih, env);
*/
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

          // Convert TWRC_XFERDONE to TWRC_SUCCESS so that appropriate value
          // is returned.

          rc = TWRC_SUCCESS;

          break;
      }
   }

   // Disable the data source.

   (*g_pDSM_Entry) (&g_AppID,
                    &srcID,
                    DG_CONTROL,
                    DAT_USERINTERFACE,
                    MSG_DISABLEDS,
                    &ui);

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
}

// ===========================================================================
// Java_net_javajeff_jtwain_JTwain_selectSourceAsDefault
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
                             g_hinstDLL,
                             0);

   // If window could not be created, throw exception.

   if (hwnd == 0)
   {
       throw TwainException("Unable to create private window "
                 "(selectSourceAsDefault)");
       return;
   }

   // Ensure that the default data source's dialog box does not disappear
   // behind other windows, which can be very disconcerting to the user. We do
   // that by making the hwnd -- created above and passed to DSM_Entry() below
   // -- the handle of the topmost window.

   SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

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

   // If data source manager could not be opened, throw exception. Because the
   // exception is not actually thrown until execution returns to Java, we
   // first exit current block to destroy previously-created window.

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

   // If failure occurred, prepare to throw an exception, which is only thrown
   // after this function ends.

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

   // Destroy the window.

   DestroyWindow (hwnd);
}

// ===========================================================================
// xferDIB8toImage
// ===========================================================================

static TwainImage xferDIB8toImage (LPBITMAPINFOHEADER lpbmih)
{
   // Obtain the image's width and height -- both in pixels -- to pass to the
   // MemoryImageSource constructor.

   int width = lpbmih->biWidth;

   int height = lpbmih->biHeight; // height < 0 if bitmap is top-down
   if (height < 0)
       height = -height;

   // Create Java-based integer pixels array to pass to the MemoryImageSource
   // constructor.

   //jintArray pixels = env->NewIntArray (width * height);
   int *pixels = new int[width * height];
   if (pixels == 0)
   {
       throw TwainException("Insufficient memory for pixels array "
                 "(xferDIB8toImage)");
   }

   // Populate the pixels array.

   int *palette = (int *) lpbmih + sizeof(BITMAPINFOHEADER);

   int numColors;

   if (lpbmih->biClrUsed > 0)
       numColors = lpbmih->biClrUsed;
   else
       numColors = 1 << lpbmih->biBitCount;
	//m_pBits = (unsigned char *)(m_pVoid) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*GetPaletteSize();
   unsigned char *bitmap = (unsigned char *) lpbmih +
                           sizeof(BITMAPINFOHEADER) +
                           numColors * sizeof(RGBQUAD);
   //int padBytes = (4 - width & 0x3) & 0x3 // this way is more efficient
   int padBytes = (4-width%4)%4; // Each pixel occupies 1 byte (palette index)
                                 // and the number of row bytes is a multiple of
                                 // 4.

   int rowBytes = width+padBytes;
   int* pixelsArray = new int[width*height];
   memcpy(pixelsArray, pixels, sizeof(int)*width * height);
   for (int row = 0; row < height; row++)
   {
        for (int col = 0; col < width; col++)
        {
             // Extract color information for pixel and build an integer array
             int pixel = 0xff000000 | palette [bitmap [rowBytes*row+col]];

             // Store the pixel in the array at the appropriate index.

             pixelsArray [width*(height-row-1)+col] = pixel;
        }
   }

   TwainImage *theImage = new TwainImage(width, height);
   theImage->setPixels(pixelsArray);

	// should return a dmtxImage
   return *theImage;
}

// ===========================================================================
// xferDIB24toImage
// ===========================================================================
/*
static jobject xferDIB24toImage (LPBITMAPINFOHEADER lpbmih, JNIEnv *env)
{
   // Obtain the image's width and height -- both in pixels -- to pass to the
   // MemoryImageSource constructor.

   int width = lpbmih->biWidth;

   int height = lpbmih->biHeight; // height < 0 if bitmap is top-down
   if (height < 0)
       height = -height;

   // Create Java-based integer pixels array to pass to the MemoryImageSource
   // constructor.

   jintArray pixels = env->NewIntArray (width * height);
   if (pixels == 0)
   {
       throwJTE (env, "Insufficient memory for pixels array "
                 "(xferDIB24toImage)");
       return (jobject) 0;
   }

   // Populate the pixels array.

   unsigned char *bitmap = (unsigned char *) lpbmih +
                           sizeof(BITMAPINFOHEADER);

   int padBytes = 3*width%4; // Each pixel occupies 3 bytes (RGB) and the
                             // number of row bytes is a multiple of 4.

   jboolean isCopy;
   jint *pixelsArray = env->GetIntArrayElements (pixels, &isCopy);
   
   if (pixelsArray == 0)
   {
       throwJTE (env, "Insufficient memory (xferDIB24toImage)");
       return (jobject) 0;
   }

   int rowBytes = width*3+padBytes;

   for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
        {
             // Obtain pixel index.

             int index = rowBytes*row+col*3;

             // Extract color information for pixel and build an equivalent
             // Java pixel for storage in the Java-based integer array.

             int pixel = 0xff000000 | (bitmap [index+2] << 16) |
                         (bitmap [index+1] << 8) | bitmap [index];

             // Store the pixel in the array at the appropriate index.

             pixelsArray [width*(height-row-1)+col] = (jint) pixel;
        }

   if (isCopy == JNI_TRUE)
       env->ReleaseIntArrayElements (pixels, pixelsArray, 0);

   // Build the equivalent of the following Java code fragement:
   //
   // MemoryImageSource mis;
   // mis = new MemoryImageSource (width, height, pixels, 0, width);
   // Image im = Toolkit.getDefaultToolkit ().createImage (mis);

   jclass clazz = env->FindClass ("java/awt/image/MemoryImageSource");
   if (clazz == 0)
   {
       throwJTE (env, "Can't find java.awt.image.MemoryImageSource class "
                 "(xferDIB24toImage)");
       return (jobject) 0;
   }

   jmethodID mid = env->GetMethodID (clazz, "<init>", "(II[III)V");
   if (mid == 0)
   {
       throwJTE (env, "Can't find java.awt.image.MemoryImageSource "
                 "constructor (xferDIB24toImage)");
       return (jobject) 0;
   }

   jobject obj1 = env->NewObject (clazz, mid, width, height, pixels, 0,
                                  width);
   if (obj1 == 0)
   {
       throwJTE (env, "Can't create java.awt.image.MemoryImageSource "
                 "object (xferDIB24toImage)");
       return (jobject) 0;
   }

   clazz = env->FindClass ("java/awt/Toolkit");
   if (clazz == 0)
   {
       throwJTE (env, "Can't find java.awt.Toolkit class (xferDIB24toImage)");
       return (jobject) 0;
   }

   mid = env->GetStaticMethodID (clazz, "getDefaultToolkit",
                                 "()Ljava/awt/Toolkit;");
   if (mid == 0)
   {
       throwJTE (env, "Can't find java.awt.Toolkit.getDefaultToolkit() "
                 "(xferDIB24toImage)");
       return (jobject) 0;
   }

   jobject obj2 = env->CallStaticObjectMethod (clazz, mid);
   if (obj2 == 0)
   {
       throwJTE (env, "Can't call java.awt.Toolkit.getDefaultToolkit() "
                 "(xferDIB24toImage)");
       return (jobject) 0;
   }

   mid = env->GetMethodID (clazz, "createImage",
                           "(Ljava/awt/image/ImageProducer;)Ljava/awt/Image;");
   if (mid == 0)
   {
       throwJTE (env, "Can't find java.awt.Toolkit.createImage() "
                 "(xferDIB24toImage)");
       return (jobject) 0;
   }

   // Return the Image subclass object reference.

   return env->CallObjectMethod (obj2, mid, obj1);
}
*/
