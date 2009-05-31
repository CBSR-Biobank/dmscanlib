/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "UaDebug.h"
#include "ImageProcessor.h"
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
	"  -v, --verbose       Debugging messages are output to stdout. Only when built\n"
	"                      UA_HAVE_DEBUG on.\n";

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

	const char * progname;
};

Application::Application(int argc, char ** argv) {
	bool AquirAndProcessImage = false;
	bool processImage = false;
	bool scanImage = false;
	bool decodeImage = false;
	char * filename = NULL;
	int ch;

	progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

	UA_DEBUG(ua::DebugSinkStdout::Instance().showHeader(true);
	ua::debugstream.sink(ua::DebugSinkStdout::Instance()));

	while (1) {
		int option_index = 0;


		ch = getopt_long (argc, argv, "ad:hp:s:v", long_options, &option_index);

		if (ch == -1) break;
		switch (ch) {
		case 'a':
			AquirAndProcessImage = true;
			break;

		case 'd':
			decodeImage = true;
			filename = optarg;
			break;

		case 'p':
			processImage = true;
			filename = optarg;
			break;

		case 's':
			scanImage = true;
			filename = optarg;
			break;

		case 200:
#ifdef WIN32
			if (!ImageGrabber::Instance().selectSourceAsDefault()) {
				cerr <<  "no scanner selected." << endl;
				exit(0);
			}
#else
			cerr << "this option not allowed on your operating system." << endl;
#endif
			break;

		case 'v':
			UA_DEBUG(ua::Debug::Instance().levelInc(ua::DebugImpl::allSubSys_m));
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

	if (processImage) {
		Dib * dib = new Dib();
		UA_ASSERT_NOT_NULL(dib);
		dib->readFromFile(filename);

		Dib * edgeDib = new Dib();
		UA_ASSERT_NOT_NULL(edgeDib);
		edgeDib->convertGrayscale(*dib);
		//edgeDib->sobelEdgeDetection(*dib);
		edgeDib->writeToFile("out.bmp");
		return;
	}

	if (AquirAndProcessImage && scanImage) {
		cerr << "Error: invalid options selected." << endl;
		exit(0);
	}

	if (decodeImage) {
		UA_ASSERT_NOT_NULL(filename);
		ImageProcessor * processor = new ImageProcessor();
		UA_ASSERT_NOT_NULL(processor);
		processor->decodeDib(filename);
	}

	if (AquirAndProcessImage) {
#ifdef WIN32
		HANDLE h = ImageGrabber::Instance().acquireImage();
		if (h == NULL) {
			cerr <<  "ERROR: could not acquire an image from scanner." << endl;
			exit(0);
		}

		Dib * dib = new Dib();
		UA_ASSERT_NOT_NULL(dib);

		dib->readFromHandle(h);

		ImageProcessor * processor = new ImageProcessor();
		UA_ASSERT_NOT_NULL(processor);
		processor->decodeDib(dib);

		ImageGrabber::Instance().freeImage(h);
		delete dib;
		delete processor;
#else
		cerr << "this option not allowed on your operating system." << endl;
#endif
	}

	if (scanImage) {
#ifdef WIN32
		UA_ASSERT_NOT_NULL(filename);
		HANDLE h = ImageGrabber::Instance().acquireImage();
		if (h == NULL) {
			cerr <<  "ERROR: could not acquire an image from scanner." << endl;
			exit(0);
		}
		Dib * dib = new Dib();
		UA_ASSERT_NOT_NULL(dib);

		dib->readFromHandle(h);
		dib->writeToFile(filename);

		ImageGrabber::Instance().freeImage(h);
		delete dib;
#else
		cerr << "this option not allowed on your operating system." << endl;
#endif
	}
}

void Application::usage() {
	printf(USAGE_FMT, progname);
}

int main(int argc, char ** argv) {
	new Application(argc, argv);
}

