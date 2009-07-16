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

const char * USAGE_FMT =
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
	"  -h, --help           Displays this text.\n"
	"  -i, --input FILE     Use the specified DIB image file instead of scanner.\n"
	"  -p, --plate NUM      The plate number to use.\n"
	"  -o, --output FILE    Saves the image to the specified file name.\n"
	"  -s, --scan           Scans an image.\n";

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = {
		{ 'b', "--brightness", SO_REQ_SEP },
		{ 'c', "--calibrate", SO_NONE },
		{ 'c', "-c",          SO_NONE },
		{ 203, "--contrast",   SO_REQ_SEP },
		{ 'd', "--decode",    SO_NONE },
		{ 'd', "-d",          SO_NONE },
		{ 200, "--debug",     SO_REQ_SEP },
		{ 201, "--dpi",       SO_REQ_SEP },
		{ 'h', "--help",      SO_NONE    },
		{ 'h', "--h",         SO_NONE    },
		{ 'i', "--input",     SO_REQ_SEP },
		{ 'i', "-i",          SO_REQ_SEP },
		{ 'p', "--plate",     SO_REQ_SEP },
		{ 'p', "-p",          SO_REQ_SEP },
		{ 'o', "--output",    SO_REQ_SEP },
		{ 'o', "-o",          SO_REQ_SEP },
		{ 's', "--scan",      SO_NONE },
		{ 's', "-s",          SO_NONE },
		{ 202, "--select",    SO_NONE },
		SO_END_OF_OPTIONS
};

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
	char * infile;
	bool help;
	unsigned plateNum;
	char * outfile;
	bool scan;
	bool select;

	Options() {
		brightness = numeric_limits<int>::max();
		calibrate = false;
		contrast = numeric_limits<int>::max();
		decode = false;
		infile = NULL;
		help = false;
		debugLevel = 0;
		dpi = 300;
		plateNum = 0;
		outfile = NULL;
		scan = false;
		select = false;
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
	progname = strrchr((char *)argv[0], DIR_SEP_CHR) + 1;

	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::logstream.sink(ua::LoggerSinkStdout::Instance());

	getCmdOptions(argc, argv);

	if (options.help) {
		usage();
		return;
	}

	int result = SC_FAIL;

	if (options.brightness != numeric_limits<int>::max()) {
		result = slConfigScannerBrightness(options.brightness);
		if (result < 0) {
			cerr << "could not assign brightness: " << options.brightness << endl;
			exit(1);
		}
	}

	if (options.contrast != numeric_limits<int>::max()) {
		result = slConfigScannerContrast(options.contrast);
		if (result < 0) {
			cerr << "could not assign contrast: " << options.contrast << endl;
			exit(1);
		}
	}

	if (options.calibrate) {
		if (options.infile != NULL) {
			calibrateToImage();
		}
		else {
			result = slCalibrateToPlate(options.dpi, options.plateNum);
		}
	}
	else if (options.decode) {
		if (options.infile != NULL) {
			result = slDecodeImage(options.plateNum, options.infile);
		}
		else {
			result = slDecodePlate(options.dpi, options.plateNum);
		}
	}
	else if (options.scan) {
		result = slScanPlate(options.dpi, options.plateNum, options.outfile);
	}
	else if (options.select) {
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

	CSimpleOptA args(argc, argv, longOptions, SO_O_NOERR|SO_O_EXACT);

	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS) {
			switch (args.OptionId()) {
			case 'b':
				options.brightness = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for brightness: " << args.OptionArg() << endl;
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
				options.plateNum = strtoul((const char *)args.OptionArg(), &end, 10);
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
				options.debugLevel = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for debug level: "
						 << args.OptionArg() << endl;
					exit(1);
				}
				ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, options.debugLevel);
				break;

			case 201:
				options.dpi = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for dpi: "
						 << args.OptionArg() << endl;
					exit(1);
				}
				break;

			case 202:
				options.select = true;
				break;

			case 203:
				options.contrast = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for contrast: " << args.OptionArg() << endl;
					exit(1);
				}
				break;

			default:
				return false;
			}
		}
		else {
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

