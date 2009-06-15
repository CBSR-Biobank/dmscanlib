/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "UaAssert.h"
#include "UaLogger.h"
#include "Decoder.h"
#include "Calibrator.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "Config.h"

#if defined (WIN32) && ! defined(__MINGW32__)
#include "ImageGrabber.h"
#else
#include <getopt.h>
#endif

using namespace std;

const char * USAGE_FMT =
	"Usage: %s [OPTIONS]\n"
	"Test tool for scanlib library."
	"\n"
	"  -c, --calibrate FILE  Calibrates decode regions to those found in bitmap FILE.\n"
	"  -d, --decode FILE     Decodes the 2D barcode in the specified DIB image file.\n"
	"  -v, --verbose NUM     Sets debugging level. Debugging messages are output "
	"                        to stdout. Only when built UA_HAVE_DEBUG on.\n";

/* Allowed command line arguments.  */
static struct option long_options[] = {
		{ "acquire",   no_argument,       NULL, 'a' },
		{ "calibrate", required_argument, NULL, 'c' },
		{ "decode",    required_argument, NULL, 'd' },
		{ "verbose",   required_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
};

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

struct Options {
	bool aquireAndProcessImage;
	bool calibrate;
	bool decodeImage;
	bool scanImage;
	char * filename;
	int plateNum;

	Options() {
		aquireAndProcessImage = false;
		calibrate = false;
		decodeImage = false;
		scanImage = false;
		filename = NULL;
	}
};

class Application {
public:
	Application(int argc, char ** argv);
	~Application();

private:
	void usage();
	void acquireAndProcesImage(unsigned plateNum);
	void calibrateToImage(char * filename);
	void decodeImage(char * filename);
	void scanImage(char * filename);

	static const char * INI_FILE_NAME;

	const char * progname;
};

const char * Application::INI_FILE_NAME = "scanlib.ini";

Application::Application(int argc, char ** argv) {
	Options options;
	int ch;

	progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::logstream.sink(ua::LoggerSinkStdout::Instance());

	while (1) {
		int option_index = 0;

		ch = getopt_long (argc, argv, "c:d:hp:tv:", long_options, &option_index);

		if (ch == -1) break;
		switch (ch) {
		case 'c':
			options.calibrate = true;
			options.filename = optarg;
			break;

		case 'd':
			options.decodeImage = true;
			options.filename = optarg;
			break;

		case 's':
			options.scanImage = true;
			options.filename = optarg;
			break;

		case 't': {
			Dib * dib = new Dib(100, 100, 24);
			UA_ASSERT_NOT_NULL(dib);
			RgbQuad quad(0, 255, 0);
			dib->line(10, 0, 80, 99, quad);
			dib->writeToFile("out.bmp");
			delete dib;
			break;
		}

		case 'v': {
			int level;
			if (!Util::strToNum(optarg, level)) {
				cerr << "not a valid number for verbose option: " << optarg << endl;
				exit(1);
			}
			ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
			break;
		}

		case '?':
		case 'h':
			usage();
			exit(0);
			break;

		default:
			cout <<  "?? getopt returned character code " << ch << " ??" << endl;
		}
	}

	if (optind < argc) {
		// too many command line arguments, print usage message
		usage();
		exit(-1);
	}


	/*
	 * Loads the file if it is present.
	 */
	if (options.calibrate) {
		calibrateToImage(options.filename);
		return;
	}
	else if (options.decodeImage) {
		decodeImage(options.filename);
		return;
	}
	else if (options.aquireAndProcessImage && options.scanImage) {
		cerr << "Error: invalid options selected." << endl;
		exit(0);
	}
}

Application::~Application() {

}

void Application::usage() {
	printf(USAGE_FMT, progname);
}

void Application::calibrateToImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	RgbQuad quad(255, 0, 0);
	Config config(INI_FILE_NAME);

	dib.readFromFile(filename);
	Dib markedDib(dib);
	Calibrator calibrator;
	if (!calibrator.processImage(dib)) {
		cout << "bad result from calibrator" << endl;
		return;
	}

	if (!config.setRegions(1, calibrator.getRowBinRegions(),
			calibrator.getColBinRegions(), calibrator.getMaxCol())) {
		cout << "bad result from config" << endl;
		return;
	}

	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("calibrated.bmp");
}

void Application::decodeImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	Config config(INI_FILE_NAME);

	dib.readFromFile(filename);
	Dib processedDib;
	processedDib.expandColours(dib, 100, 200);
	processedDib.writeToFile("blurred.bmp");
	exit(0);

	Dib markedDib(dib);
	Decoder decoder;
	if (!config.parseRegions(1)) {
		cout << "bad result from config" << endl;
		return;
	}
	decoder.processImageRegions(1, dib, config.getRegions());
	decoder.imageShowRegions(markedDib, config.getRegions());
	markedDib.writeToFile("decoded.bmp");
	config.saveDecodeResults(1);
}

int main(int argc, char ** argv) {
	Application app(argc, argv);
}

