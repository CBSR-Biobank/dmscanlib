/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "UaDebug.h"
#include "Decoder.h"
#include "Calibrator.h"
#include "Dib.h"

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#ifdef WIN32
#include "getopt.h"
#include "ImageGrabber.h"
#else
#include <getopt.h>
#endif

#include <iostream>
#include <fstream>

using namespace std;

const char * USAGE_FMT =
	"Usage: %s [OPTIONS]\n"
	"\n"
	"  -a, --acquire         Scans an image from the scanner and decodes the 2D barcodes\n"
	"                        on the trays.\n"
	"  -c, --calibrate FILE  Calibrates decode regions to those found in bitmap FILE.\n"
	"  -d, --decode FILE     Decodes the 2D barcode in the specified DIB image file.\n"
	"  -p, --process FILE    Attempts to find all trays in image.\n"
	"  -s, --scan FILE       Scans an image and saves it as a DIB.\n"
	"  --select              User will be asked to select the scanner.\n"
	"  -v, --verbose NUM     Sets debugging level. Debugging messages are output "
	"                        to stdout. Only when built UA_HAVE_DEBUG on.\n";

/* Allowed command line arguments.  */
static struct option long_options[] = {
		{ "acquire",   no_argument,       NULL, 'a' },
		{ "calibrate", required_argument, NULL, 'c' },
		{ "decode",    required_argument, NULL, 'd' },
		{ "process",   required_argument, NULL, 'p' },
		{ "scan",      required_argument, NULL, 's' },
		{ "select",    no_argument,       NULL, 200 },
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
	bool processImage;
	bool scanImage;
	char * filename;

	Options() {
		aquireAndProcessImage = false;
		calibrate = false;
		decodeImage = false;
		processImage = false;
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
	void acquireAndProcesImage();
	void calibrateToImage(char * filename);
	void decodeImage(char * filename);
	void processImage(char * filename);
	void scanImage(char * filename);

	static const char * INI_FILE_NAME;

	const char * progname;
	CSimpleIniA ini;
};

const char * Application::INI_FILE_NAME = "scanlib.ini";

Application::Application(int argc, char ** argv) :
	ini(true, false, true) {
	Options options;
	int ch;

	progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

	UA_DEBUG(ua::DebugSinkStdout::Instance().showHeader(true);
	ua::debugstream.sink(ua::DebugSinkStdout::Instance()));

	while (1) {
		int option_index = 0;


		ch = getopt_long (argc, argv, "ac:d:hp:s:tv:", long_options, &option_index);

		if (ch == -1) break;
		switch (ch) {
		case 'a':
			options.aquireAndProcessImage = true;
			break;

		case 'c':
			options.calibrate = true;
			options.filename = optarg;
			break;

		case 'd':
			options.decodeImage = true;
			options.filename = optarg;
			break;

		case 'p':
			options.processImage = true;
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

		case 200: {
#ifdef WIN32
			const char * err;
			if (!ImageGrabber::Instance().selectSourceAsDefault(&err)) {
				cerr <<  err << endl;
				exit(0);
			}
#else
			cerr << "this option not allowed on your operating system." << endl;
#endif
			break;
		}

		case 'v':
			UA_DEBUG(ua::Debug::Instance().levelSet(
					ua::DebugImpl::allSubSys_m, atoi(optarg)));
			break;

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
	fstream inifile;
	inifile.open(INI_FILE_NAME, fstream::in);
	if (inifile.is_open()) {
		SI_Error rc = ini.Load(inifile);
		UA_ASSERTS(rc >= 0, "attempt to load ini file returned: " << rc);
		inifile.close();
	}

	if (options.calibrate) {
		calibrateToImage(options.filename);
		return;
	}
	else if (options.processImage) {
		processImage(options.filename);
		return;
	}
	else if (options.aquireAndProcessImage && options.scanImage) {
		cerr << "Error: invalid options selected." << endl;
		exit(0);
	}
	else if (options.processImage) {
		processImage(options.filename);
	}
	else if (options.aquireAndProcessImage) {
		acquireAndProcesImage();
	}
	else if (options.scanImage) {
		scanImage(options.filename);
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
	DmtxVector2 p00, p10, p11, p01;

	dib.readFromFile(filename);
	Dib markedDib(dib);
	Calibrator calibrator;
	calibrator.processDib(dib);
	calibrator.debugShowTags();
	calibrator.saveRegionsToIni(ini);
	ini.SaveFile(INI_FILE_NAME);

	unsigned numTags = calibrator.getNumTags();
	UA_DOUT(1, 3, "marking tags ");
	for (unsigned i = 0; i < numTags; ++i) {
		calibrator.getTagCorners(i, p00, p10, p11, p01);
		UA_DOUT(1, 9, "marking tag " << i);
		markedDib.line((unsigned) p00.X, (unsigned) p00.Y,
				(unsigned) p10.X, (unsigned) p10.Y, quad);
		markedDib.line((unsigned) p10.X, (unsigned) p10.Y,
				(unsigned) p11.X, (unsigned) p11.Y, quad);
		markedDib.line((unsigned) p11.X, (unsigned) p11.Y,
				(unsigned) p01.X, (unsigned) p01.Y, quad);
		markedDib.line((unsigned) p01.X, (unsigned) p01.Y,
				(unsigned) p00.X, (unsigned) p00.Y, quad);
	}
	markedDib.writeToFile("out.bmp");

	cout << calibrator.getResults();
}

void Application::decodeImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	RgbQuad quad(255, 0, 0);
	DmtxVector2 p00, p10, p11, p01;

	dib.readFromFile(filename);
	Dib markedDib(dib);
	Decoder decoder;
	decoder.getRegionsFromIni(ini);
	decoder.processImageRegions(dib);

	unsigned numTags = decoder.getNumTags();
	UA_DOUT(1, 3, "marking tags ");
	for (unsigned i = 0; i < numTags; ++i) {
		decoder.getTagCorners(i, p00, p10, p11, p01);
		UA_DOUT(1, 9, "marking tag " << i);
		markedDib.line((unsigned) p00.X, (unsigned) p00.Y,
				(unsigned) p10.X, (unsigned) p10.Y, quad);
		markedDib.line((unsigned) p10.X, (unsigned) p10.Y,
				(unsigned) p11.X, (unsigned) p11.Y, quad);
		markedDib.line((unsigned) p11.X, (unsigned) p11.Y,
				(unsigned) p01.X, (unsigned) p01.Y, quad);
		markedDib.line((unsigned) p01.X, (unsigned) p01.Y,
				(unsigned) p00.X, (unsigned) p00.Y, quad);
	}
	markedDib.writeToFile("out.bmp");

	cout << decoder.getResults();
}

void Application::processImage(char * filename) {
	Dib dib;
	dib.readFromFile(filename);

	Dib edgeDib;
	edgeDib.crop(dib, 2590, 644, 3490, 1950);
	//edgeDib->sobelEdgeDetection(*dib);
	edgeDib.writeToFile("out.bmp");

	Decoder decoder(dib);
	decoder.debugShowTags();
}

void Application::acquireAndProcesImage() {
#ifdef WIN32
	const char * err;

	HANDLE h = ImageGrabber::Instance().acquireImage(&err);
	if (h == NULL) {
		cerr <<  err << endl;
		exit(0);
	}

	Dib dib;
	Decoder decoder;

	dib.readFromHandle(h);
	decoder.debugShowTags();
	ImageGrabber::Instance().freeImage(h);
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}

void Application::scanImage(char * filename) {
#ifdef WIN32
	const char * err;

	UA_ASSERT_NOT_NULL(filename);
	HANDLE h = ImageGrabber::Instance().acquireImage(&err);
	if (h == NULL) {
		cerr <<  err << endl;
		exit(0);
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ImageGrabber::Instance().freeImage(h);
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}

int main(int argc, char ** argv) {
	Application app(argc, argv);
}

