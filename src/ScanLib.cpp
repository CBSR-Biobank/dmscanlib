#ifdef _VISUALC_
#include "ImageGrabber.h"
#endif

//#include <getopt.h>
#include "ImageProcessor.h"
#include <iostream>

/* Allowed command line arguments.  */
/*static struct option long_options[] = {
   { "decode",    no_argument, NULL, 'd' },
   { 0, 0, 0, 0 }
};

const char * USAGE_FMT =
   "Usage: %s [OPTIONS]\n";

char * progname;

void printUsage() {
   printf(USAGE_FMT, progname);
}
*/

int main(int argc, char ** argv) {
   int ch;
/*
   progname = argv[0];

   while (1) {
      int option_index = 0;

      ch = getopt_long (argc, argv, "d:h", long_options, &option_index);

      if (ch == -1) break;
      switch (ch) {

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
   */
	char* name = argv[1];
	std::cout << name;
   decodeDib(name);
}
