/*
 * DmScanLibCommon.h
 *
 *  Created on: 2011-07-18
 *      Author: thomas
 */

#ifndef DMSCANLIBCOMMON_H_
#define DMSCANLIBCOMMON_H_

class Dib;

void configLogging(unsigned level, bool useFile);

int slDecodeCommon(unsigned plateNum, Dib & dib, double scanGap,
		unsigned squareDev, unsigned edgeThresh, unsigned corrections,
		double cellDistance, double gapX, double gapY, unsigned profileA,
		unsigned profileB, unsigned profileC, unsigned isVertical,
		const char *markedDibFilename);

int slDecodeImage(unsigned verbose, unsigned plateNum, const char *filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned isVertical);


#endif /* DMSCANLIBCOMMON_H_ */
