/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "ScanLib.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "Decoder.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "SimpleOpt.h"

#include <iostream>

#ifdef WIN32
#include "ImageGrabber.h"
#endif

using namespace std;

const char
* USAGE_FMT =
   "Usage: %s [OPTIONS]\n"
   "Test tool for scanlib library."
   "\n"
   "  --brightness NUM     The brightness setting to be used for scanning.\n"
   "  --debug NUM          Sets debugging level. Debugging messages are output\n"
   "                       to stdout. Only when built UA_HAVE_DEBUG on.\n"
   "  --celldist NUM       Distance between tube cells in inches.\n"
   "  --corrections NUM    The number of corrections to make.\n"
   "  --contrast NUM       The contrast setting to be used for scanning.\n"
   "  -d, --decode         Acquires an image from the scanner and Decodes the 2D barcodes.\n"
   "                       Use with --plate option.\n"
   "  --dpi NUM            Dots per inch to use with scanner.\n"
   "  --gap NUM            Use scan grid with gap of N pixels (or less) between lines.\n"
   "  -h, --help           Displays this text.\n"
   "  -i, --input FILE     Use the specified DIB image file instead of scanner.\n"
   "  -p, --plate NUM      The plate number to use.\n"
   "  --processImage       Perform image processing on image before decoding.\n"
   "  -o, --output FILE    Saves the image to the specified file name.\n"
   "  -s, --scan           Scans an image.\n"
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
   "\n"
   "Scanning Coordinates\n"
   "  -l, --left NUM       The left coordinate, in inches, for the scanning window.\n"
   "  -t, --top NUM        The top coordinate, in inches, for the scanning window.\n"
   "  -r, --right NUM      The left coordinate, in inches, for the scanning window.\n"
   "  -b, --bottom NUM     The bottom coordinate, in inches, for the scanning window.\n";

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = {
   { 205, "--brightness", SO_REQ_SEP },
   { 207, "--celldist", SO_REQ_SEP },
   { 206, "--corrections", SO_REQ_SEP },
   { 203, "--contrast", SO_REQ_SEP },
   { 'd', "--decode", SO_NONE },
   { 'd', "-d", SO_NONE },
   { 200, "--debug", SO_REQ_SEP },
   { 201, "--dpi", SO_REQ_SEP },
   { 300, "--gap", SO_REQ_SEP },
   { 'h', "--help", SO_NONE },
   { 'h', "--h", SO_NONE },
   { 'i', "--input", SO_REQ_SEP },
   { 'i', "-i", SO_REQ_SEP },
   { 'p', "--plate", SO_REQ_SEP },
   { 'p', "-p", SO_REQ_SEP },
   { 204, "--processImage", SO_NONE },
   { 'o', "--output", SO_REQ_SEP },
   { 'o', "-o", SO_REQ_SEP },
   { 's', "--scan", SO_NONE },
   { 's', "-s", SO_NONE },
   { 202, "--select", SO_NONE },
   { 301, "--square-dev", SO_REQ_SEP },
   { 302, "--threshold", SO_REQ_SEP },
   { 400, "--left",  SO_REQ_SEP },
   { 401, "--top",  SO_REQ_SEP },
   { 402, "--right",  SO_REQ_SEP },
   { 403, "--bottom",  SO_REQ_SEP },
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
   int brightness;
   double cellDistance;
   int corrections;
   int contrast;
   bool decode;
   unsigned debugLevel;
   unsigned dpi;
   double gap;
   bool help;
   char * infile;
   unsigned plateNum;
   char * outfile;
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
      corrections = 0;
      contrast = 9999;
#else
      brightness = numeric_limits<int>::max();
      contrast = numeric_limits<int>::max();
#endif 

      cellDistance = 0.33;
      decode = false;
      debugLevel = 0;
      dpi = 300;
      gap = 0.0;
      help = false;
      infile = NULL;
      plateNum = 0;
      outfile = NULL;
      scan = false;
      select = false;
      squareDev = 10;
      threshold = 50;
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

const char * Application::INI_FILE_NAME = "scanlib.ini";

Application::Application(int argc, char ** argv) {
   progname = strrchr((char *) argv[0], DIR_SEP_CHR) + 1;

   ua::LoggerSinkStdout::Instance().showHeader(true);
   ua::logstream.sink(ua::LoggerSinkStdout::Instance());

   getCmdOptions(argc, argv);

   if (options.help) {
      usage();
      return;
   }

   int result = SC_FAIL;

   configLogging(options.debugLevel, false);

   if (options.decode) {
      if (options.infile != NULL) {
         result = slDecodeImage(options.debugLevel, options.plateNum,
                                options.infile, options.gap, options.squareDev,
                                options.threshold, options.corrections,
                                options.cellDistance);
      } else {
         result = slDecodePlate(options.debugLevel, options.dpi,
                 options.brightness, options.contrast, options.plateNum,
                 options.left, options.top, options.right, options.bottom,
                 options.gap, options.squareDev, options.threshold,
                 options.corrections, options.cellDistance);
      }
   } else if (options.scan) {
      if ((options.plateNum < 1) || (options.plateNum > 5)) {
         result = slScanImage(options.debugLevel, options.dpi,
                              options.brightness, options.contrast, options.left, options.top,
                              options.right, options.bottom,
                              options.outfile);
      }
   } else if (options.select) {
      result = slSelectSourceAsDefault();
   }

   cout << "return code is: " << result << endl;
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
            case 205:
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

            case 200:
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

            case 201:
               options.dpi
                  = strtoul((const char *) args.OptionArg(), &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for dpi: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case 202:
               options.select = true;
               break;

            case 203:
               options.contrast = strtoul((const char *) args.OptionArg(),
                                          &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for contrast: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case 206:
               options.corrections = strtoul((const char *) args.OptionArg(),
                                          &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for corrections: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case 301:
               options.squareDev = strtoul((const char *) args.OptionArg(),
                                           &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for gap: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case 302:
               options.threshold = strtoul((const char *) args.OptionArg(),
                                           &end, 10);
               if (*end != 0) {
                  cerr << "invalid value for gap: " << args.OptionArg()
                       << endl;
                  exit(1);
               }
               break;

            case 207:
            case 300:
            case 'l':
            case 't':
            case 'r':
            case 'b':
            case 400:
            case 401:
            case 402:
            case 403: {
               double num = strtod((const char *) args.OptionArg(),
                                   &end);
               if (*end != 0) {
                  cerr << "invalid value for " << args.OptionId() << ": "
                       << args.OptionArg() << endl;
                  exit(1);
               }

               if ((args.OptionId() == 'l') || (args.OptionId() == 400)) {
                  options.left = num;
               }
               else if ((args.OptionId() == 't') || (args.OptionId() == 401)) {
                  options.top = num;
               }
               else if ((args.OptionId() == 'r') || (args.OptionId() == 402)) {
                  options.right = num;
               }
               else if ((args.OptionId() == 'b') || (args.OptionId() == 403)) {
                  options.bottom = num;
               }
               else if (args.OptionId() == 207) {
                  options.cellDistance = num;
               }
               else if (args.OptionId() == 300) {
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

int main(int argc, char ** argv) {
   Application app(argc, argv);
}

