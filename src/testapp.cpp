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
	"  -c, --calibrate NUM  Calibrates decode regions for plate NUM.\n"
	"  -d, --decode NUM     Decodes the 2D barcode for plate NUM.\n"
	"  -i, --input FILE     Use the specified DIB image file instead of scanner.\n"
	"  --debug NUM          Sets debugging level. Debugging messages are output "
	"                       to stdout. Only when built UA_HAVE_DEBUG on.\n";

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = {
		{ 'c', "--calibrate", SO_REQ_SEP },
		{ 'c', "-c",          SO_REQ_SEP },
		{ 'd', "--decode",    SO_REQ_SEP },
		{ 'd', "-d",          SO_REQ_SEP },
		{ 200, "--debug",     SO_REQ_SEP },
		{ 'i', "--input",     SO_REQ_SEP },
		{ 'i', "-i",          SO_REQ_SEP },
		SO_END_OF_OPTIONS
};

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif


struct Options {
	bool calibrate;
	bool decode;
	int  debugLevel;
	char * filename;
	bool help;
	unsigned plateNum;

	Options() {
		calibrate = false;
		decode = false;
		filename = NULL;
		help = false;
		debugLevel = 0;
		plateNum = 0;
	}
};

class Application {
public:
	Application(int argc, char ** argv);
	~Application();

private:
	void usage();
	bool getCmdOptions(int argc, char ** argv);
	void acquireAndProcesImage(unsigned plateNum);
	void calibrateToImage(char * filename);
	void decodeImage(char * filename);

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

	/*
	 * Loads the file if it is present.
	 */
	if (options.calibrate) {
		if (options.filename != NULL) {
			calibrateToImage(options.filename);
		}
		else {
			slCalibrateToPlate(options.plateNum);
		}
	}
	else if (options.decode) {
		if (options.filename != NULL) {
			decodeImage(options.filename);
		}
		else {
			slDecodePlate(options.plateNum);
		}
	}
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
			case 'c':
				options.calibrate = true;
				options.plateNum = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for plate number: "
						 << args.OptionArg() << endl;
					exit(1);
				}
				break;

			case 'd':
				options.decode = true;
				options.plateNum = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for plate number: "
						 << args.OptionArg() << endl;
					exit(1);
				}
				break;

			case 'h':
				options.help = true;
				break;

			case 'i':
				options.filename = args.OptionArg();
				break;

			case 200:
				options.debugLevel = strtoul((const char *)args.OptionArg(), &end, 10);
				if (*end != 0) {
					cerr << "invalid value for plate number: "
						 << args.OptionArg() << endl;
					exit(1);
				}
				ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, options.debugLevel);
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

void Application::calibrateToImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib, processedDib;
	RgbQuad quad(255, 0, 0);
	Config config(INI_FILE_NAME);

	dib.readFromFile(filename);
	processedDib.gaussianBlur(dib);
	processedDib.unsharp(dib);
	//processedDib.expandColours(dib, 150, 220);
	processedDib.writeToFile("processed.bmp");

	Calibrator calibrator;
	if (!calibrator.processImage(processedDib)) {
		cout << "bad result from calibrator" << endl;
		return;
	}

	if (!config.setRegions(1, calibrator.getRowBinRegions(),
			calibrator.getColBinRegions(), calibrator.getMaxCol())) {
		cout << "bad result from config" << endl;
		return;
	}

	Dib markedDib(dib);
	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("calibrated.bmp");
}

void Application::decodeImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib, processedDib;
	Config config(INI_FILE_NAME);

	dib.readFromFile(filename);
	processedDib.gaussianBlur(dib);
	processedDib.unsharp(dib);
	//processedDib.expandColours(dib, 150, 220);
	processedDib.writeToFile("processed.bmp");
	//exit(0);

	Dib markedDib(dib);
	Decoder decoder;
	if (!config.parseRegions(1)) {
		cout << "bad result from config" << endl;
		return;
	}
	decoder.processImageRegions(1, processedDib, config.getRegions());
	decoder.imageShowRegions(markedDib, config.getRegions());
	markedDib.writeToFile("decoded.bmp");
	config.saveDecodeResults(1);
}

int main(int argc, char ** argv) {
	Application app(argc, argv);
}

