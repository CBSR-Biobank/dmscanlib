#ifndef __INC_SCANLIB_H
#define __INC_SCANLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#ifdef BUILD_DLL
/* DLL export */
#define EXPORT __declspec(dllexport)
#else
/* EXE import */
#define EXPORT __declspec(dllimport)
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

const unsigned short SC_SUCCESS      = 0;
const unsigned short SC_FAIL         = 1;
const unsigned short SC_TWAIN_UAVAIL = 2;

EXPORT unsigned short slIsTwainAvailable();

typedef unsigned short (FAR PASCAL *SL_ISTWAINAVAILABLE) ();

EXPORT unsigned short slSelectSourceAsDefault();

typedef unsigned short (FAR PASCAL *SL_SELECTSOURCEASDEFAULT) ();

EXPORT unsigned short slConfigPlateFrame(unsigned short plateNum, double left,
		double top,	double right, double bottom);

typedef unsigned short (FAR PASCAL *SL_CONFIGPLATEFRAME) (unsigned short, double x0,
		double y0,	double x1, double y1);

EXPORT unsigned short slScanImage(char * filename, double left,	double top,
		double right, double bottom);

typedef unsigned short (FAR PASCAL *SL_SCANIMAGE) (char * filename, double x0,
		double y0,	double x1, double y1);

EXPORT unsigned short slCalibrateToPlate(unsigned short plateNum);

typedef unsigned short (FAR PASCAL *SL_CALIBRATETOPLATE) (unsigned int plateNum);

EXPORT unsigned short slDecodePlate(unsigned short plateNum);

typedef unsigned short (FAR PASCAL *SL_DECODEPLATE) (unsigned int plateNum);

#ifdef __cplusplus
}
#endif

#endif /* __INC_SCANLIB_H */
