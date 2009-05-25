/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "UaDebug.h"
#include "ImageProcessor.h"

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

const char * USAGE_FMT =
	"Usage: %s [OPTIONS]\n"
	"\n"
	"  -a, --acquire       Scans an image from the scanner and decodes the 2D barcodes\n"
	"                      on the trays.\n"
	"  -d, --decode FILE   Decodes the 2D barcode in the specified DIB image file.\n"
	"  -s, --scan FILE     Scans an image and saves it as a DIB.\n"
	"  -v, --verbose       Debugging messages are output to stdout. Only when built\n"
	"                      UA_HAVE_DEBUG on.\n";

/* Allowed command line arguments.  */
static struct option long_options[] = {
		{ "decode",  no_argument, NULL, 'd' },
		{ "acquire", no_argument, NULL, 'a' },
		{ "verbose", required_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
};

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

char * progname;

void printUsage() {
	printf(USAGE_FMT, progname);
}

int main(int argc, char ** argv) {
	int ch;
	progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

	UA_DEBUG(ua::DebugSinkStdout::Instance().showHeader(true);
	ua::debugstream.sink(ua::DebugSinkStdout::Instance()));

	while (1) {
		int option_index = 0;

		ch = getopt_long (argc, argv, "ad:hs:v", long_options, &option_index);

		if (ch == -1) break;
		switch (ch) {
		case 'a': {
#ifdef WIN32
			ImageGrabber grabber = new ImageGrabber();
			UA_ASSERT_NOT_NULL(grabber);
			grabber->selectSourceAsDefault();
			ImageProcessor * processor = new ImageProcessor();
			UA_ASSERT_NOT_NULL(processor);
			HANDLE h = grabber->acquireImage();
			Dib dib = new Dib();
			dib->readFromHandle(h)
			processor->decodeImage(dib);
			grabber->freeImage(h);
#endif
			break;
		}

		case 'd': {
			ImageProcessor * processor = new ImageProcessor();
			UA_ASSERT_NOT_NULL(processor);
			processor->decodeDib(optarg);
			break;
		}

		case 's': {
#ifdef WIN32
			Dib dib = new Dib();
			UA_ASSERT_NOT_NULL(dib);

			ImageGrabber grabber = new ImageGrabber();
			UA_ASSERT_NOT_NULL(grabber);

			grabber->selectSourceAsDefault();
			HANDLE h = grabber->acquireImage();
			dib->readFromHandle(h);
			dib->writeToFile(optarg);
			grabber->freeImage(h);
#endif
			break;
		}

		case 'v':
			UA_DEBUG(ua::Debug::Instance().levelInc(ua::DebugImpl::allSubSys_m));
			break;

		case '?':
		case 'h':
			printUsage();
			exit(0);
			break;

		default:
			printf("?? getopt returned character code %c ??\n", ch);
		}
	}

	if (optind > argc) {
		// too many command line arguments, print usage message
		printUsage();
		exit(-1);
	}
}
