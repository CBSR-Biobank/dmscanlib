/*
 * TestApp.h
 *
 *  Created on: 2011-07-18
 *      Author: thomas
 */

#ifndef TESTAPP_H_
#define TESTAPP_H_



#include <limits>
#include <stdio.h>
using namespace std;

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
    bool isVertical;
	bool flatbed;

	Options();

};


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


#endif /* TESTAPP_H_ */
