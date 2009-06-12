#ifndef __INC_SCANLIB_H
#define __INC_SCANLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <windows.h>

#ifdef BUILD_DLL
/* DLL export */
#define EXPORT __declspec(dllexport)
#else
/* EXE import */
#define EXPORT __declspec(dllimport)
#endif
#else
#define EXPORT
#define FAR
#define PASCAL
#endif


typedef struct sScPixelLoc {
	int x;
	int y;
} ScPixelLoc;

typedef struct sScPixelFrame {
	int x0; // top
	int y0; // left
	int x1; // bottom
	int y1; // right
} ScPixelFrame;

/**
 * Specifies the region to scan.
 */
typedef struct sScFrame {
	int frameId;
	double x0; // top
	double y0; // left
	double x1; // bottom
	double y1; // right
} ScFrame;

const short SC_SUCCESS      = 0;
const short SC_FAIL         = -1;
const short SC_TWAIN_UAVAIL = -2;
const short SC_CALIBRATOR_NO_REGIONS = -3;
const short SC_INI_FILE_ERROR = -4;

EXPORT short slIsTwainAvailable();

typedef short (FAR PASCAL *SL_ISTWAINAVAILABLE) ();

EXPORT short slSelectSourceAsDefault();

typedef short (FAR PASCAL *SL_SELECTSOURCEASDEFAULT) ();

EXPORT short slConfigPlateFrame(unsigned short plateNum, double left,
		double top,	double right, double bottom);

typedef short (FAR PASCAL *SL_CONFIGPLATEFRAME) (unsigned short, double x0,
		double y0,	double x1, double y1);

EXPORT short slScanImage(char * filename, double left,	double top,
		double right, double bottom);

typedef short (FAR PASCAL *SL_SCANIMAGE) (char * filename, double x0,
		double y0,	double x1, double y1);

EXPORT short slCalibrateToPlate(unsigned short plateNum);

typedef short (FAR PASCAL *SL_CALIBRATETOPLATE) (unsigned short plateNum);

EXPORT short slDecodePlate(unsigned short plateNum);

typedef short (FAR PASCAL *SL_DECODEPLATE) (unsigned short plateNum);

#ifdef __cplusplus
}
#endif

#endif /* __INC_SCANLIB_H */
