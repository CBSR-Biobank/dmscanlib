#include "ImageProcessor.h"
#include <iostream>

/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ImageProcessor.h"

#ifdef _VISUALC_
#include "utils\getopt.h"
#include "ImageGrabber.h"
#else
#include "ImageGrabber.h"
#include <getopt.h>
#endif

/* Allowed command line arguments.  */
static struct option long_options[] = {

   { "decode",    no_argument, NULL, 'd' },
   { "acquire", no_argument, NULL, 'a' },
   { 0, 0, 0, 0 }
};

const char * USAGE_FMT =
   "Usage: %s [OPTIONS]\n";

#ifdef _VISUALC_
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
	DmtxImage *theImage;
   progname = strrchr(argv[0], DIR_SEP_CHR) + 1;

   while (1) {
      int option_index = 0;

	  ch = getopt_long (argc, argv, "a:d:h", long_options, &option_index);

      if (ch == -1) break;
      switch (ch) {
		 case 'a':
			initGrabber();
			selectSourceAsDefault();
			theImage = acquire();
			decodeDmtxImage(theImage);
			dmtxImageDestroy(&theImage);
			 break;
         case 'd':
            decodeDib(optarg);
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

   if (optind >= argc) {
      // not enough command line arguments, print usage message
      printUsage();
      exit(-1);
   }

   if (optind > argc) {
      // too many command line arguments, print usage message
      printUsage();
      exit(-1);
   }
}
