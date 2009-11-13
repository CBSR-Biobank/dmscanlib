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
#include "Calibrator.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "Config.h"
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
					"  -c, --calibrate      Acquires an image from the scanner. Use with --plate option.\n"
					"  --debug NUM          Sets debugging level. Debugging messages are output\n"
					"                       to stdout. Only when built UA_HAVE_DEBUG on.\n"
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
					"                       done, but may be necessary for low contrast or blurry images.\n";

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = { { 'b', "--brightness", SO_REQ_SEP }, {
		'c', "--calibrate", SO_NONE }, { 'c', "-c", SO_NONE }, { 203,
		"--contrast", SO_REQ_SEP }, { 'd', "--decode", SO_NONE }, { 'd', "-d",
		SO_NONE }, { 200, "--debug", SO_REQ_SEP },
		{ 201, "--dpi", SO_REQ_SEP }, { 300, "--gap", SO_REQ_SEP }, { 'h',
				"--help", SO_NONE }, { 'h', "--h", SO_NONE }, { 'i', "--input",
				SO_REQ_SEP }, { 'i', "-i", SO_REQ_SEP }, { 'p', "--plate",
				SO_REQ_SEP }, { 'p', "-p", SO_REQ_SEP }, { 204,
				"--processImage", SO_NONE }, { 'o', "--output", SO_REQ_SEP }, {
				'o', "-o", SO_REQ_SEP }, { 's', "--scan", SO_NONE }, { 's',
				"-s", SO_NONE }, { 202, "--select", SO_NONE }, { 301,
				"--square-dev", SO_REQ_SEP },
		{ 302, "--threshold", SO_REQ_SEP }, SO_END_OF_OPTIONS };

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

struct Options {
	int brightness;
	bool calibrate;
	int contrast;
	bool decode;
	unsigned debugLevel;
	unsigned dpi;
	unsigned gap;
	bool help;
	char * infile;
	unsigned plateNum;
	char * outfile;
	bool scan;
	bool select;
	unsigned squareDev;
	unsigned threshold;

	Options() {
		brightness = numeric_limits<int>::max();
		calibrate = false;
		contrast = numeric_limits<int>::max();
		decode = false;
		debugLevel = 0;
		dpi = 300;
		gap = 0;
		help = false;
		infile = NULL;
		plateNum = 0;
		outfile = NULL;
		scan = false;
		select = false;
		squareDev = 10;
		threshold = 50;
	}
};

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

	if (options.calibrate) {
		if (options.infile != NULL) {
			calibrateToImage();
		} else {
			result = slCalibrateToPlate(options.dpi, options.plateNum);
		}
	} else if (options.decode) {
		if (options.infile != NULL) {
			result = slDecodeImage(options.debugLevel, options.plateNum, options.infile,
					options.gap, options.squareDev, options.threshold);
		} else {
			result = slDecodePlate(options.debugLevel, options.dpi, options.plateNum,
					options.brightness, options.contrast, options.gap,
					options.squareDev, options.threshold);
		}
	} else if (options.scan) {
		if ((options.plateNum < 1) || (options.plateNum > 5)) {
			result = slScanImage(options.debugLevel, options.dpi, options.brightness,
					options.contrast, 0, 0, 20, 20, options.outfile);
		} else {
			result = slScanPlate(options.debugLevel, options.dpi, options.plateNum,
					options.brightness, options.contrast, options.outfile);
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
			case 'b':
				options.brightness = strtoul((const char *) args.OptionArg(),
						&end, 10);
				if (*end != 0) {
					cerr << "invalid value for brightness: "
							<< args.OptionArg() << endl;
					exit(1);
				}
				break;

			case 'c':
				options.calibrate = true;
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

			case 300:
				options.gap
						= strtoul((const char *) args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for gap: " << args.OptionArg()
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

void Application::calibrateToImage() {
	UA_ASSERT_NOT_NULL(options.infile);

	Dib dib, processedDib;
	RgbQuad quad(255, 0, 0);
	Config config(INI_FILE_NAME);

	dib.readFromFile(options.infile);
	processedDib.gaussianBlur(dib);
	processedDib.unsharp(dib);
	//processedDib.expandColours(dib, 150, 220);
	processedDib.writeToFile("processed.bmp");

	Calibrator calibrator;
	if (!calibrator.processImage(processedDib)) {
		cout << "bad result from calibrator" << endl;
		return;
	}

	if (!config.setRegions(1, dib.getDpi(), calibrator.getRowBinRegions(),
			calibrator.getColBinRegions())) {
		cout << "bad result from config" << endl;
		return;
	}

	Dib markedDib(dib);
	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("calibrated.bmp");
}

int main(int argc, char ** argv) {
	Application app(argc, argv);
}

