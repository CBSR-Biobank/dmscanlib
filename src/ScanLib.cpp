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
#include "Dib.h"

#ifdef WIN32
#include "getopt.h"
#include "ImageGrabber.h"
#else
#include <getopt.h>
#endif

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

const char * USAGE_FMT =
	"Usage: %s [OPTIONS]\n"
	"\n"
	"  -a, --acquire       Scans an image from the scanner and decodes the 2D barcodes\n"
	"                      on the trays.\n"
	"  -d, --decode FILE   Decodes the 2D barcode in the specified DIB image file.\n"
	"  -p, --process FILE  Attempts to find all trays in image.\n"
	"  -s, --scan FILE     Scans an image and saves it as a DIB.\n"
	"  --select            User will be asked to select the scanner.\n"
	"  -v, --verbose NUM   Sets debugging level. Debugging messages are output "
	"                      to stdout. Only when built UA_HAVE_DEBUG on.\n";

/* Allowed command line arguments.  */
static struct option long_options[] = {
		{ "acquire", no_argument,       NULL, 'a' },
		{ "decode",  required_argument, NULL, 'd' },
		{ "process", required_argument, NULL, 'p' },
		{ "scan",    required_argument, NULL, 's' },
		{ "select",  no_argument,       NULL, 200 },
		{ "verbose", required_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
};

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

class Application {
public:
	Application(int argc, char ** argv);
	~Application();

private:
	void usage();
	void decodeImage(char * filename);
	void processImage(char * filename);
	void acquireAndProcesImage();
	void scanImage(char * filename);

	const char * progname;
};

struct Options {
	bool aquireAndProcessImage;
	bool processImage;
	bool scanImage;
	bool decodeImage;
	char * filename;

	Options() {
		aquireAndProcessImage = false;
		processImage = false;
		scanImage = false;
		decodeImage = false;
		filename = NULL;
	}
};

Application::Application(int argc, char ** argv) {
	Options options;
	int ch;

	progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

	UA_DEBUG(ua::DebugSinkStdout::Instance().showHeader(true);
	ua::debugstream.sink(ua::DebugSinkStdout::Instance()));

	while (1) {
		int option_index = 0;


		ch = getopt_long (argc, argv, "ad:hp:s:tv:", long_options, &option_index);

		if (ch == -1) break;
		switch (ch) {
		case 'a':
			options.aquireAndProcessImage = true;
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

	if (options.processImage) {
		processImage(options.filename);
		return;
	}
	else if (options.aquireAndProcessImage && options.scanImage) {
		cerr << "Error: invalid options selected." << endl;
		exit(0);
	}
	else if (options.decodeImage) {
		decodeImage(options.filename);
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

void Application::decodeImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	dib.readFromFile(filename);

	Decoder decoder(dib);
	decoder.debugShowTags();

	RgbQuad quad(255, 0, 0);

	Dib markedDib(dib);
	DmtxVector2 p00, p10, p11, p01;
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

