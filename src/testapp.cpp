/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#ifdef _VISUALC_
#   define _CRTDBG_MAP_ALLOC
#   pragma warning(disable : 4996)
     //Scan for memory leaks in visual studio
#   ifdef _DEBUG
#      define _CRTDBG_MAP_ALLOC
#      include <stdlib.h>
#      include <crtdbg.h>
#   endif
#endif

#include <iostream>
#include <bitset>


#include <afxwin.h>
#include "cv.h"
#include "highgui.h"

// Main blob library include
#include "cvblob/include/BlobResult.h"

char wndname[] = "Blob Extraction";
char tbarname1[] = "Threshold";
char tbarname2[] = "Blob Size";

// The output and temporary images
IplImage* originalThr = 0;
IplImage* original = 0;
IplImage* filtered = 0;
IplImage* displayedImage = 0;

int param1,param2;



// threshold trackbar callback
void on_trackbar( int dummy )
{
	if(!originalThr)
	{
		originalThr = cvCreateImage(cvGetSize(original), IPL_DEPTH_8U,1);
	}

	if(!displayedImage)
	{
		displayedImage = cvCreateImage(cvGetSize(original), IPL_DEPTH_8U,3);
	}
	
	// threshold input image
	cvThreshold( filtered, originalThr, param1, 255, CV_THRESH_BINARY );

	// get blobs and filter them using its area
	CBlobResult blobs;
	int i;
	CBlob *currentBlob;

	// find blobs in image
	blobs = CBlobResult( originalThr, NULL, 0 );
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, param2 );

	// display filtered blobs
	cvMerge( original, original, original, NULL, displayedImage );

	for (i = 0; i < blobs.GetNumBlobs(); i++ )
	{

		currentBlob = blobs.GetBlob(i);

		//currentBlob->FillBlob( displayedImage, CV_RGB(255,0,0));
		
		//cvCircle(displayedImage, cvPoint((int)currentBlob->GetEllipse().center.x,(int)currentBlob->GetEllipse().center.y), (int)(currentBlob->GetEllipse().size.width+currentBlob->GetEllipse().size.height)/2,CV_RGB(0,255,255), 2, 8, 0);

		int border = 0;

		CvPoint pt1, pt2;
		pt1.x = currentBlob->GetBoundingBox().x -border;
		pt1.y = currentBlob->GetBoundingBox().y -border;
		pt2.x = currentBlob->GetBoundingBox().x + currentBlob->GetBoundingBox().width + border;
		pt2.y = currentBlob->GetBoundingBox().y + currentBlob->GetBoundingBox().height + border;

		cvRectangle(
				displayedImage,
				pt1,pt2,
				CV_RGB(0,255,0),3, 8,0);


	}
    cvShowImage( wndname, displayedImage );
}


void applyPlateFilters(IplImage * img){

	int width     = img->width;
	int height    = img->height;
	int depth     = img->depth;
	int nchannels = img->nChannels;

	// memory leaking?
	IplImage* tmp = cvCreateImage(cvSize( width, height ),depth, nchannels );

	for(int i=0; i < 4; i++){
		cvSmooth(img,tmp,CV_GAUSSIAN,11,11);
		cvSmooth(tmp,img,CV_GAUSSIAN,11,11);
	}
	cvSmooth(img,tmp,CV_GAUSSIAN,11,11);
}



int main( int argc, char** argv )
{
	if(argc < 2){
		exit(-1);
	}

	param1 = 50;
	param2 = 2000;
	
	original  = cvLoadImage(argv[1],0);
	filtered  = cvLoadImage(argv[1],0);

	cvNamedWindow("input",0);
	cvResizeWindow("input",800,600);
	cvShowImage("input", original );

	applyPlateFilters(filtered);

	cvNamedWindow("filtered",0);
	cvResizeWindow("filtered",800,600);
	cvShowImage("filtered", filtered );

	cvNamedWindow(wndname, 0);
	cvResizeWindow(wndname,800,600);
    cvCreateTrackbar( tbarname1, wndname, &param1, 255, on_trackbar );
	cvCreateTrackbar( tbarname2, wndname, &param2, 30000, on_trackbar );


	
	// Call to update the view
	for(;;)
    {
        int c;
        
        // Call to update the view
        on_trackbar(0);

        c = cvWaitKey(0);

	   if( c == 27 )
            break;
	}
    
    cvReleaseImage( &original );
	cvReleaseImage( &originalThr );
	cvReleaseImage( &displayedImage );
    
    cvDestroyWindow( wndname );
    
    return 0;
}




#ifndef WIN32
#   include <limits>
#   include <bitset>
#   include <vector>
#   include <stdio.h>
#endif


#ifdef USE_NVWA
#   include "debug_new.h"
#endif

#ifdef WIN32
#   include "UaAssert.h"
#   include "Decoder.h"
#   include "Dib.h"
#   include "Util.h"
#   include "BarcodeInfo.h"
#   include "ImageGrabber.h"
#endif

#include "ScanLib.h"
#include "SimpleOpt.h"
#include "UaLogger.h"

using namespace std;

const char * USAGE_FMT =
   "Usage: %s [OPTIONS]\n"
   "Test tool for scanlib library."
   "\n"
   "  --debug NUM          Sets debugging level. Debugging messages are output\n"
   "                       to stdout. Only when built UA_HAVE_DEBUG on.\n"
   "  --debugfile          Send debugging output to file named scanlib.log.\n"
   "  --select             Opens the default scanner dialog.\n"
   "  --capability         Query selected scanner for dpi and driver type settings.\n"
   "  --test			   Tests most functions in this project.\n"
   "  -h, --help           Displays this text.\n"
   "\n"
   "Scanner/Decoding Settings\n"
   "  -d, --decode         Acquires an image from the scanner and Decodes the 2D barcodes.\n"
   "                       Use with --plate option.\n"
   "  -s, --scan           Scans an image.\n"
   "  -p, --plate NUM      The plate number to use.\n"
   "  -i, --input FILE     Use the specified DIB image file instead of scanner.\n"
   "  -o, --output FILE    Saves the image to the specified file name.\n"
   "\n"
   "  --dpi NUM            Dots per inch to use with scanner.\n"
   "  --brightness NUM     The brightness setting to be used for scanning.\n"
   "  --contrast NUM       The contrast setting to be used for scanning.\n"
   "  --square-dev NUM     Maximum  deviation  (degrees)  from  squareness between adjacent\n"
   "                       barcode sides. Default value is N=40, but  N=10  is  recommended\n"
   "                       for  flat  applications  like faxes and other scanned documents.\n"
   "                       Barcode regions found with corners <(90-N) or  >(90+N)  will  be\n"
   "                       ignored by the decoder.\n"
   "  --threshold NUM      Set  the  minimum edge threshold as a percentage of maximum. For\n"
   "                       example, an edge between a pure white and pure black pixel would\n"
   "                       have  an  intensity  of  100.  Edges  with intensities below the\n"
   "                       indicated threshold will be ignored  by  the  decoding  process.\n"
   "                       Lowering  the  threshold  will increase the amount of work to be\n"
   "                       done, but may be necessary for low contrast or blurry images.\n"
   "  --corrections NUM    The number of corrections to make.\n"
   "  --celldist NUM       Distance between tube cells in inches.\n"
   "  --gap NUM            Use scan grid with gap of NUM inches (or less) between lines.\n"
   "\n"
   "Scanning Coordinates\n"
   "  -l, --left NUM       The left coordinate, in inches, for the scanning window.\n"
   "  -t, --top NUM        The top coordinate, in inches, for the scanning window.\n"
   "  -r, --right NUM      The left coordinate, in inches, for the scanning window.\n"
   "  -b, --bottom NUM     The bottom coordinate, in inches, for the scanning window.\n";


enum longOptID {
	OPT_ID_BRIGHTNESS = 200,
			OPT_ID_CELLDIST,
			OPT_ID_CORRECTIONS,
			OPT_ID_CONTRAST,
			OPT_ID_DEBUG,
			OPT_ID_DEBUG_FILE,
			OPT_ID_DPI,
			OPT_ID_GAP,
			OPT_ID_SELECT,
			OPT_ID_CAPABILITY,
			OPT_ID_TEST,
			OPT_ID_SQUARE_DEV,
			OPT_ID_THRESHOLD,
			OPT_ID_LEFT,
			OPT_ID_TOP,
			OPT_ID_RIGHT,
			OPT_ID_BOTTOM
};

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = {
   { OPT_ID_BRIGHTNESS, "--brightness", SO_REQ_SEP },
   { OPT_ID_CELLDIST, "--celldist", SO_REQ_SEP },
   { OPT_ID_CORRECTIONS, "--corrections", SO_REQ_SEP },
   { OPT_ID_CONTRAST, "--contrast", SO_REQ_SEP },
   { 'd', "--decode", SO_NONE },
   { 'd', "-d", SO_NONE },
   { OPT_ID_DEBUG, "--debug", SO_REQ_SEP },
   { OPT_ID_DEBUG_FILE, "--debugfile", SO_NONE },
   { OPT_ID_DPI, "--dpi", SO_REQ_SEP },
   { OPT_ID_GAP, "--gap", SO_REQ_SEP },
   { 'h', "--help", SO_NONE },
   { 'h', "--h", SO_NONE },
   { 'i', "--input", SO_REQ_SEP },
   { 'i', "-i", SO_REQ_SEP },
   { 'p', "--plate", SO_REQ_SEP },
   { 'p', "-p", SO_REQ_SEP },
   { 'o', "--output", SO_REQ_SEP },
   { 'o', "-o", SO_REQ_SEP },
   { 's', "--scan", SO_NONE },
   { 's', "-s", SO_NONE },
   { OPT_ID_CAPABILITY, "--capability", SO_NONE },
   { OPT_ID_TEST, "--test", SO_NONE },
   { OPT_ID_SELECT, "--select", SO_NONE },
   { OPT_ID_SQUARE_DEV, "--square-dev", SO_REQ_SEP },
   { OPT_ID_THRESHOLD, "--threshold", SO_REQ_SEP },
   { OPT_ID_LEFT, "--left",  SO_REQ_SEP },
   { OPT_ID_TOP, "--top",  SO_REQ_SEP },
   { OPT_ID_RIGHT, "--right",  SO_REQ_SEP },
   { OPT_ID_BOTTOM, "--bottom",  SO_REQ_SEP },
   { 'l', "-l",  SO_REQ_SEP },
   { 't', "-t",  SO_REQ_SEP },
   { 'r', "-r",  SO_REQ_SEP },
   { 'b', "-b",  SO_REQ_SEP },
   SO_END_OF_OPTIONS
};

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

struct Options {
   std::vector<unsigned> dpis;
   char * infile;
   char * outfile;

   int brightness;
   double cellDistance;
   int corrections;
   int contrast;
   bool decode;
   unsigned debugLevel;
   bool debugfile;
   double gap;
   bool help;
   bool capability;
   bool test;
   unsigned plateNum;
   bool scan;
   bool select;
   unsigned squareDev;
   unsigned threshold;
   double left;
   double top;
   double right;
   double bottom;

   Options() {

#ifdef WIN32
      brightness = 9999;
      contrast = 9999;
#else
      brightness = numeric_limits<int>::max();
      contrast = numeric_limits<int>::max();
#endif 

      decode = false;
	  debugfile = false;
      debugLevel = 0;

      help = false;
      scan = false;
      select = false;
	  capability = false;
	  test = false;

	  infile = NULL;
      outfile = NULL;
	  plateNum = 0;

	  cellDistance = 0.345;
	  gap = 0.085;
	  corrections = 10;
	  squareDev = 15;
      threshold = 5;

      left = 0.0;
      top = 0.0;
      right = 0.0;
	  bottom = 0.0;
   }
};

// this function defined in ScanLib.cpp and is only used by test application
void configLogging(unsigned level, bool useFile);

class Application {
public:
   Application(int argc, char ** argv);
   ~Application();

private:
   void usage();
   bool getCmdOptions(int argc, char ** argv);
   void acquireAndProcesImage();
   void calibrateToImage();

   static const char * INI_FILE_NAME;
   const char * progname;

   Options options;
};

int location = 0;

const char * Application::INI_FILE_NAME = "scanlib.ini";

Application::Application(int argc, char ** argv) {

   progname = strrchr((char *) argv[0], DIR_SEP_CHR) + 1;

   getCmdOptions(argc, argv);

   ua::LoggerSinkStdout::Instance().showHeader(true);
   if (!options.debugfile) {
	   ua::logstream.sink(ua::LoggerSinkStdout::Instance());
   }
   configLogging(options.debugLevel, options.debugfile);

   if (options.help) {
      usage();
      return;
   }

   int result = SC_FAIL;
   unsigned numDpis = options.dpis.size();


   if (options.decode ) {
	   if (options.infile != NULL) {
		   result = slDecodeImage(options.debugLevel, 
								  options.plateNum,
								  options.infile, 
								  options.gap, 
								  options.squareDev,
								  options.threshold, 
								  options.corrections,
								  options.cellDistance);
	   } 
	   else if (numDpis == 1) {

		   result = slDecodePlate(options.debugLevel, 
								  options.dpis[0],
								  options.brightness, 
								  options.contrast, 
								  options.plateNum,
								  options.left, 
								  options.top, 
								  options.right, 
								  options.bottom,
								  options.gap, 
								  options.squareDev, 
								  options.threshold,
								  options.corrections, 
								  options.cellDistance);

	   } 
	   else if ((numDpis == 2) || (numDpis == 3)) {

		   if (numDpis == 2) 
			   options.dpis.push_back(0);
		   
		   result = slDecodePlateMultipleDpi(options.debugLevel, 
											 options.dpis[0],
											 options.dpis[1], 
											 options.dpis[2],
											 options.brightness, 
											 options.contrast, 
											 options.plateNum,
											 options.left, 
											 options.top, 
											 options.right, 
											 options.bottom,
											 options.gap, 
											 options.squareDev, 
											 options.threshold,
											 options.corrections, 
											 options.cellDistance);
		}

   }/* Decode */

   else if (options.scan) {
	   if ((options.plateNum >= 1) && (options.plateNum <= 5) && (options.dpis.size() > 0)) {

		   result = slScanImage(options.debugLevel, 
								options.dpis[0],
								options.brightness, 
								options.contrast, 
								options.left, 
								options.top,
								options.right, 
								options.bottom,
								options.outfile);
	   }


   }/* Scan */

   else if (options.select) {
	   result = slSelectSourceAsDefault();
	    
   }/* Select */

   else if (options.capability){
		
	   bitset<sizeof(int)> bits(slGetScannerCapability());
		
	    cout << "Capability code: " << bits.to_string<char,char_traits<char>,allocator<char> >() << endl;
		result = SC_SUCCESS;
   }

   else if (options.test) {

	   if(options.dpis.size() < 3 ||
		  (options.left == 0.0 && options.right == 0.0 && options.top == 0.0 && options.bottom == 0.0)){
		
		  cout << "Must provide 3 dpi values and plate offsets."<< endl;
		  result =  SC_FAIL;
		  goto STOP;
	   }
	   cout << "Initializing Test Procedure" << endl << endl;


	   cout << "==============Select Source================" << endl;
	   result = slSelectSourceAsDefault();
	   if(result != SC_SUCCESS){
			cout << "Failed to select source."<< endl;
			goto STOP;
	   }
	   cout << "===========================================" << endl;

	   cout << "==============Scan Image to File================" << endl;
	   result = slScanImage(options.debugLevel, 
								options.dpis[0],
								options.brightness, 
								options.contrast, 
								options.left, 
								options.top,
								options.right, 
								options.bottom,
								"test.bmp");

	   if(result != SC_SUCCESS){
			cout << "Failed to scan image to file."<< endl;
			goto STOP;
	   }
	   cout << "===========================================" << endl;

	   cout << "==============Decode image from file================" << endl;
	   result = slDecodeImage(options.debugLevel, 
								  1,
								  "test.bmp",
								  options.gap, 
								  options.squareDev,
								  options.threshold, 
								  options.corrections,
								  options.cellDistance);

	   if(result != SC_SUCCESS){
			cout << "Failed to decode scanned image file."<< endl;
			goto STOP;
	   }

	   cout << "===========================================" << endl;

	   cout << "==============Scan & Decode Image================" << endl;
	   result = slDecodePlate(options.debugLevel, 
								  options.dpis[0],
								  options.brightness, 
								  options.contrast, 
								  1,
								  options.left, 
								  options.top, 
								  options.right, 
								  options.bottom,
								  options.gap, 
								  options.squareDev, 
								  options.threshold,
								  options.corrections, 
								  options.cellDistance);

	   if(result != SC_SUCCESS){
			cout << "Failed to scan & decode image."<< endl;
			goto STOP;
	   }

	   cout << "===========================================" << endl;

	   
	   cout << "==============Scan & Decode Multiple Dpi Image================" << endl;
	   result = slDecodePlateMultipleDpi(options.debugLevel, 
											 options.dpis[0],
											 options.dpis[1], 
											 options.dpis[2],
											 options.brightness, 
											 options.contrast, 
											 1,
											 options.left, 
											 options.top, 
											 options.right, 
											 options.bottom,
											 options.gap, 
											 options.squareDev, 
											 options.threshold,
											 options.corrections, 
											 options.cellDistance);

	   if(result != SC_SUCCESS){
			cout << "Failed to scan & decode multiple dpi imaeg"<< endl;
			goto STOP;
	   }

	   cout << "===========================================" << endl;




STOP: ;
	   
   		#ifdef _VISUALC_
		#ifdef _DEBUG
		_CrtDumpMemoryLeaks();
		#endif
		#endif
   }


   switch(result){
	
		case SC_SUCCESS:
			cout << "return code is: SC_SUCCESS"<< endl;
			break;
		case SC_FAIL:
			cout << "return code is: SC_FAIL"<< endl;
			break;
		case SC_TWAIN_UAVAIL:
			cout << "return code is: SC_TWAIN_UAVAIL"<< endl;
			break;
		case SC_INVALID_DPI:
			cout << "return code is: SC_INVALID_DPI"<< endl;
			break;
		case SC_INVALID_PLATE_NUM:
			cout << "return code is: SC_INVALID_PLATE_NUM"<< endl;
			break;
		case SC_INVALID_VALUE:
			cout << "return code is: SC_INVALID_VALUE"<< endl;
			break;
		case SC_INVALID_IMAGE:
			cout << "return code is: SC_INVALID_IMAGE"<< endl;
			break;
		case SC_INVALID_POSITION:
			cout << "return code is: SC_INVALID_POSITION"<< endl;
			break;
		case SC_POS_CALC_ERROR:
			cout << "return code is: SC_POS_CALC_ERROR"<< endl;
			break;
		case SC_INCORRECT_DPI_SCANNED:
			cout << "return code is: SC_INCORRECT_DPI_SCANNED"<< endl;
			break;
   }
   cout << "return int-code is: " << result << endl;
}

Application::~Application() {

}

void Application::usage() {
   printf(USAGE_FMT, progname);
}

bool Application::getCmdOptions(int argc, char ** argv) {
   char * end = 0;

   CSimpleOptA args(argc, argv, longOptions, SO_O_NOERR | SO_O_EXACT);

   while (args.Next()) {
      if (args.LastError() == SO_SUCCESS) {
         switch (args.OptionId()) {
            case OPT_ID_BRIGHTNESS:
               options.brightness = strtoul((const char *) args.OptionArg(),
                                            &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for brightness: "
                       << args.OptionArg() << endl;
                  exit(1);
               }
               break;

            case 'd':
               options.decode = true;
               break;

            case 'h':
               options.help = true;
               break;

            case 'i':
               options.infile = args.OptionArg();
               break;

            case 'p':
               options.plateNum = strtoul((const char *) args.OptionArg(),
                                          &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for plate number: "
                       << args.OptionArg() << endl;
                  exit(1);
               }
               break;

            case 'o':
               options.outfile = args.OptionArg();
               break;

            case 's':
               options.scan = true;
               break;

            case OPT_ID_DEBUG:
               options.debugLevel = strtoul((const char *) args.OptionArg(),
                                            &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for debug level: "
                       << args.OptionArg() << endl;
                  exit(1);
               }
               ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m,
                                               options.debugLevel);
               break;

            case OPT_ID_DPI:
               options.dpis.push_back(
                  strtoul((const char *) args.OptionArg(), &end, 10));
               if (*end != 0) {
                  cerr << "invalid value for dpi: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case OPT_ID_SELECT:
               options.select = true;
               break;

		    case OPT_ID_CAPABILITY:
				options.capability = true;
               break;

			case OPT_ID_TEST:
				options.test = true;
               break;

            case OPT_ID_CONTRAST:
               options.contrast = strtoul((const char *) args.OptionArg(),
                                          &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for contrast: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case OPT_ID_CORRECTIONS:
               options.corrections = strtoul((const char *) args.OptionArg(),
                                          &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for corrections: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case OPT_ID_DEBUG_FILE:
            	options.debugfile = true;
            	break;

            case OPT_ID_SQUARE_DEV:
               options.squareDev = strtoul((const char *) args.OptionArg(),
                                           &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for gap: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case OPT_ID_THRESHOLD:
               options.threshold = strtoul((const char *) args.OptionArg(),
                                           &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for gap: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case OPT_ID_CELLDIST:
            case OPT_ID_GAP:
            case 'l':
            case 't':
            case 'r':
            case 'b':
            case OPT_ID_LEFT:
            case OPT_ID_TOP:
            case OPT_ID_RIGHT:
            case OPT_ID_BOTTOM: {
               double num = strtod((const char *) args.OptionArg(),
                                   &end);
               if (*end != 0) {
                  cerr << "invalid value for " << args.OptionId() << ": "
                       << args.OptionArg() << endl;
                  exit(1);
               }

               if ((args.OptionId() == 'l') || (args.OptionId() == OPT_ID_LEFT)) {
                  options.left = num;
               }
               else if ((args.OptionId() == 't') || (args.OptionId() == OPT_ID_TOP)) {
                  options.top = num;
               }
               else if ((args.OptionId() == 'r') || (args.OptionId() == OPT_ID_RIGHT)) {
                  options.right = num;
               }
               else if ((args.OptionId() == 'b') || (args.OptionId() == OPT_ID_BOTTOM)) {
                  options.bottom = num;
               }
               else if (args.OptionId() == OPT_ID_CELLDIST) {
                  options.cellDistance = num;
               }
               else if (args.OptionId() == OPT_ID_GAP) {
                  options.gap = num;
               }
               break;
            }

            default:
               return false;
         }
      } else {
         cerr << "Invalid argument: " << args.OptionText() << endl;
         return false;
      }
   }
   return true;
}
/*
int main(int argc, char ** argv) {
   Application app(argc, argv);
}

// main.cpp : Defines the entry point for the console application.
//
*/

