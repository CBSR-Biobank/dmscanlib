/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "ImageProcessor.h"

#include "UaDebug.h"
#include "ImageProcessor.h"

#ifdef _VISUALC_
#include "getopt.h"
#include "ImageGrabber.h"
#else
#include <getopt.h>
#endif

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Allowed command line arguments.  */
static struct option long_options[] = {

   { "decode",  no_argument, NULL, 'd' },
   { "acquire", no_argument, NULL, 'a' },
   { "verbose", required_argument, NULL, 'v' },
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

   UA_DEBUG(ua::DebugSinkStdout::Instance().showHeader(true);
             ua::debugstream.sink(ua::DebugSinkStdout::Instance()));

   while (1) {
      int option_index = 0;

      ch = getopt_long (argc, argv, "a:d:hv", long_options, &option_index);

      if (ch == -1) break;
      switch (ch) {
         case 'a':
#ifdef _VISUALC_
            initGrabber();
            selectSourceAsDefault();
            theImage = acquire();
            decodeDmtxImage(theImage);
            dmtxImageDestroy(&theImage);
#endif
            break;
         case 'd':
            decodeDib(optarg);
            break;

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
