#ifndef __INC_DM_SCAN_LIB_SIMULATE_H
#define __INC_DM_SCAN_LIB_SIMULATE_H

#include "DmScanLib.h"
#include "DmScanLibInternal.h"

class DmScanLibSimulate : public DmScanLib {
public:
	DmScanLibSimulate(unsigned debugLevel, bool haveDebugFile);

	virtual ~DmScanLibSimulate();

	int isTwainAvailable();

	int selectSourceAsDefault();

	int getScannerCapability();

	int isValidDpi(int dpi);

	int scanImage(unsigned verbose, unsigned dpi, int brightness, int contrast,
	        double left, double top, double right, double bottom,
	        const char *filename);

	int decodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
	        unsigned plateNum, double left, double top, double right,
	        double bottom, double scanGap, unsigned squareDev, unsigned edgeThresh,
	        unsigned corrections, double cellDistance, double gapX, double gapY,
	        unsigned profileA, unsigned profileB, unsigned profileC,
	        unsigned isVertical);

	int scanFlatbed(unsigned verbose, unsigned dpi, int brightness, int contrast,
	        const char *filename);

private:
};

#endif /* __INC_DM_SCAN_LIB_SIMULATE_H */
