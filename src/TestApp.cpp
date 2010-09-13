/*
 Dmscanlib is a software library and standalone application that scans 
 and decodes libdmtx compatible test-tubes. It is currently designed 
 to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 Copyright (C) 2010 Canadian Biosample Repository

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
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

#ifndef WIN32
#   include <limits>
#   include <stdio.h>
#endif

#ifdef WIN32
#   include "UaAssert.h"
#   include "Decoder.h"
#   include "Dib.h"
#   include "Util.h"
#   include "BarcodeInfo.h"
#   include "ImageGrabber.h"
#endif

#include "DmScanLib.h"
#include "SimpleOpt.h"
#include "UaLogger.h"

#include <iostream>
#include <bitset>
#include <vector>

#ifdef USE_NVWA
#   include "debug_new.h"
#endif

using namespace std;

const char
		* USAGE_FMT =
				"Usage: %s [OPTIONS]\n"
					"Test tool for dmscanlib library."
					"\n"
					"  --debug NUM          Sets debugging level. Debugging messages are output\n"
					"                       to stdout. Only when built UA_HAVE_DEBUG on.\n"
					"  --debugfile          Send debugging output to file named dmscanlib.log.\n"
					"  --select             Opens the scanner selection dialog.\n"
					"  --capability         Query selected scanner for DPI and driver type settings.\n"
					"  -h, --help           Displays this text.\n"
					"\n"
					"Scanner/Decoding Settings\n"
					"  -d, --decode         Acquires an image from the scanner and Decodes the 2D barcodes.\n"
					"                       Use with --plate option.\n"
					"  --super				Super decode common.\n"
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
					"  -v					Image is rotated veritcally. (default is horiztonal orientation)\n"
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
	OPT_ID_SUPER,
	OPT_ID_SQUARE_DEV,
	OPT_ID_THRESHOLD,
	OPT_ID_LEFT,
	OPT_ID_TOP,
	OPT_ID_RIGHT,
	OPT_ID_BOTTOM
};

/* Allowed command line arguments.  */
CSimpleOptA::SOption longOptions[] = { { OPT_ID_BRIGHTNESS, "--brightness",
		SO_REQ_SEP }, { OPT_ID_CELLDIST, "--celldist", SO_REQ_SEP }, {
		OPT_ID_CORRECTIONS, "--corrections", SO_REQ_SEP }, { OPT_ID_CONTRAST,
		"--contrast", SO_REQ_SEP }, { 'd', "--decode", SO_NONE }, { 'd', "-d",
		SO_NONE }, { OPT_ID_DEBUG, "--debug", SO_REQ_SEP }, {
		OPT_ID_DEBUG_FILE, "--debugfile", SO_NONE }, { OPT_ID_DPI, "--dpi",
		SO_REQ_SEP }, { OPT_ID_GAP, "--gap", SO_REQ_SEP }, { 'h', "--help",
		SO_NONE }, { 'h', "--h", SO_NONE }, { 'i', "--input", SO_REQ_SEP }, {
		'i', "-i", SO_REQ_SEP }, { 'p', "--plate", SO_REQ_SEP }, { 'p', "-p",
		SO_REQ_SEP }, { 'o', "--output", SO_REQ_SEP },
		{ 'o', "-o", SO_REQ_SEP }, { 's', "--scan", SO_NONE }, { 's', "-s",
				SO_NONE }, { OPT_ID_CAPABILITY, "--capability", SO_NONE }, {
				OPT_ID_SELECT, "--select", SO_NONE }, { OPT_ID_SQUARE_DEV,
				"--square-dev", SO_REQ_SEP }, { OPT_ID_THRESHOLD,
				"--threshold", SO_REQ_SEP }, { OPT_ID_LEFT, "--left",
				SO_REQ_SEP }, { OPT_ID_TOP, "--top", SO_REQ_SEP }, {
				OPT_ID_RIGHT, "--right", SO_REQ_SEP }, { OPT_ID_BOTTOM,
				"--bottom", SO_REQ_SEP }, { 'l', "-l", SO_REQ_SEP }, { 't',
				"-t", SO_REQ_SEP }, { 'r', "-r", SO_REQ_SEP }, { 'b', "-b",
				SO_REQ_SEP }, SO_END_OF_OPTIONS };

#ifdef WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

struct Options {
	unsigned dpi;
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
	double gapX;
	double gapY;
	unsigned profileA;
	unsigned profileB;
	unsigned profileC;
	bool isHoriztonal;

	Options() {

#ifdef WIN32
		brightness = 9999;
		contrast = 9999;
#else
		brightness = numeric_limits<int>::max();
		contrast = numeric_limits<int>::max();
#endif

		dpi = 0;
		decode = false;
		debugfile = false;
		debugLevel = 0;

		help = false;
		scan = false;
		select = false;
		capability = false;
		test = false;
		isHoriztonal = true;

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

		gapX = 0;
		gapY = 0;
		profileA = (unsigned) -1;
		profileB = (unsigned) -1;
		profileC = (unsigned) -1;
	}
};

// this function defined in DmScanLib.cpp and is only used by test application
void configLogging(unsigned level, bool useFile);

class TestApp {
public:
	TestApp(int argc, char ** argv);
	~TestApp();

private:
	void usage();
	bool getCmdOptions(int argc, char ** argv);
	void acquireAndProcesImage();
	void calibrateToImage();
	int decode();
	int scan();
	int capability();
	int test();

	static const char * INI_FILE_NAME;
	const char * progname;

	Options options;
};

int location = 0;

const char * TestApp::INI_FILE_NAME = "dmscanlib.ini";

TestApp::TestApp(int argc, char ** argv) {

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

	if (options.decode) {
		result = decode();
	}
	if (options.scan) {
		result = scan();
	} else if (options.select) {
		result = slSelectSourceAsDefault();
	} else if (options.capability) {
		result = capability();
	}

	switch (result) {

	case SC_SUCCESS:
		cout << "return code is: SC_SUCCESS" << endl;
		break;
	case SC_FAIL:
		cout << "return code is: SC_FAIL" << endl;
		break;
	case SC_TWAIN_UAVAIL:
		cout << "return code is: SC_TWAIN_UAVAIL" << endl;
		break;
	case SC_INVALID_DPI:
		cout << "return code is: SC_INVALID_DPI" << endl;
		break;
	case SC_INVALID_PLATE_NUM:
		cout << "return code is: SC_INVALID_PLATE_NUM" << endl;
		break;
	case SC_INVALID_VALUE:
		cout << "return code is: SC_INVALID_VALUE" << endl;
		break;
	case SC_INVALID_IMAGE:
		cout << "return code is: SC_INVALID_IMAGE" << endl;
		break;
	case SC_INVALID_POSITION:
		cout << "return code is: SC_INVALID_POSITION" << endl;
		break;
	case SC_POS_CALC_ERROR:
		cout << "return code is: SC_POS_CALC_ERROR" << endl;
		break;
	case SC_INCORRECT_DPI_SCANNED:
		cout << "return code is: SC_INCORRECT_DPI_SCANNED" << endl;
		break;
	}
	cout << "return int-code is: " << result << endl;
}

TestApp::~TestApp() {

}

int TestApp::decode() {
	if (options.infile != NULL) {
		return slDecodeImage(options.debugLevel, options.plateNum,
				options.infile, options.gap, options.squareDev,
				options.threshold, options.corrections, options.cellDistance,
				options.gapX, options.gapY, options.profileA, options.profileB,
				options.profileC, options.isHoriztonal);
	}

	if ((options.left == 0.0) && (options.right == 0.0) && (options.top == 0.0)
			&& (options.bottom == 0.0)) {
		cout << "ERROR: Must provide plate offsets." << endl;
		return SC_FAIL;
	}

	if (options.dpi == 0) {
		cerr << "ERROR: DPI not specified" << endl;
		return SC_FAIL;
	}

	return slDecodePlate(options.debugLevel, options.dpi, options.brightness,
			options.contrast, options.plateNum, options.left, options.top,
			options.right, options.bottom, options.gap, options.squareDev,
			options.threshold, options.corrections, options.cellDistance,
			options.gapX, options.gapY, options.profileA, options.profileB,
			options.profileC, options.isHoriztonal);
}

int TestApp::scan() {

	if (options.dpi == 0) {
		cerr << "ERROR: DPI not specified" << endl;
		return SC_FAIL;
	}

	return slScanImage(options.debugLevel, options.dpi, options.brightness,
			options.contrast, options.left, options.top, options.right,
			options.bottom, options.outfile);
}

int TestApp::capability() {
	unsigned caps = slGetScannerCapability();

	if ((caps & 0x01) == 0) {
		cout << "  Scanner is TWAIN" << endl;
	}
	if ((caps & 0x01) == 1) {
		cout << "  Scanner is WIA" << endl;
	}

	if ((caps & 0x02) == 0) {
		cout << "  300 DPI not supported" << endl;
	}
	if ((caps & 0x02) != 0) {
		cout << "  300 DPI supported" << endl;
	}

	if ((caps & 0x04) == 0) {
		cout << "  400 DPI not supported" << endl;
	}
	if ((caps & 0x04) != 0) {
		cout << "  400 DPI supported" << endl;
	}

	if ((caps & 0x08) == 0) {
		cout << "  600 DPI not supported" << endl;
	}
	if ((caps & 0x08) != 0) {
		cout << "  600 DPI supported" << endl;
	}

	if ((caps & 0x10) == 0) {
		cout << "  no scanner found" << endl;
	}
	return SC_SUCCESS;
}

void TestApp::usage() {
	printf(USAGE_FMT, progname);
}

bool TestApp::getCmdOptions(int argc, char ** argv) {
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

			case 'v':
				options.isHoriztonal = false;
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
				options.dpi
						= strtoul((const char *) args.OptionArg(), &end, 10);
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
					cerr << "invalid value for corrections: "
							<< args.OptionArg() << endl;
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
				double num = strtod((const char *) args.OptionArg(), &end);
				if (*end != 0) {
					cerr << "invalid value for " << args.OptionId() << ": "
							<< args.OptionArg() << endl;
					exit(1);
				}

				if ((args.OptionId() == 'l')
						|| (args.OptionId() == OPT_ID_LEFT)) {
					options.left = num;
				} else if ((args.OptionId() == 't') || (args.OptionId()
						== OPT_ID_TOP)) {
					options.top = num;
				} else if ((args.OptionId() == 'r') || (args.OptionId()
						== OPT_ID_RIGHT)) {
					options.right = num;
				} else if ((args.OptionId() == 'b') || (args.OptionId()
						== OPT_ID_BOTTOM)) {
					options.bottom = num;
				} else if (args.OptionId() == OPT_ID_CELLDIST) {
					options.cellDistance = num;
				} else if (args.OptionId() == OPT_ID_GAP) {
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
#if defined(_VISUALC_) && defined(_DEBUG)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	TestApp app(argc, argv);
}

