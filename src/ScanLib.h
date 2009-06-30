#ifndef __INC_SCANLIB_H
#define __INC_SCANLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) && defined(BUILD_DLL) && ! defined(STANDALONE)
    /* DLL export */
#   define EXPORT __declspec(dllexport)
#elif defined(WIN32) && ! defined(STANDALONE)
    /* EXE import */
#   define EXPORT __declspec(dllimport)
#else
#   define EXPORT
#endif


#ifdef WIN32
#   include <windows.h>
#endif

/**
 * Return codes used by the DLL API.
 */
const int SC_SUCCESS               =  0;
const int SC_FAIL                  = -1;
const int SC_TWAIN_UAVAIL          = -2;
const int SC_CALIBRATOR_NO_REGIONS = -3;
const int SC_CALIBRATOR_ERROR      = -4;
const int SC_INI_FILE_ERROR        = -5;
const int SC_INVALID_DPI           = -6;
const int SC_INVALID_PLATE_NUM     = -7;
const int SC_FILE_SAVE_ERROR       = -8;
const int SC_INVALID_VALUE         = -9;

/**
 * Queries the availability of the TWAIN driver.
 *
 * @return SC_SUCCESS if available, and SC_INVALID_VALUE if unavaiable.
 */
EXPORT int slIsTwainAvailable();

typedef int (*SL_IS_TWAIN_AVAILABLE) ();

EXPORT int slSelectSourceAsDefault();

typedef int (*SL_SELECT_SOURCE_AS_DEFAULT) ();

/**
 * Brightness value to be used for scanning.
 *
 * @param brightness a value between -1000 and 1000.
 *
 * @return SC_SUCCESS if valid. SC_INVALID_VALUE if bad value.
 */
EXPORT int slConfigScannerBrightness(int brightness);

typedef int (*SL_CONFIG_SCANNER_BRIGHTNESS) (int brightness);

/**
 * Contrast value to be used for scanning.
 *
 * @param contrast a value between -1000 and 1000.
 *
 * @return SC_SUCCESS if valid. SC_INVALID_VALUE if bad value.
 */
EXPORT int slConfigScannerContrast(int contrast);

typedef int (*SL_CONFIG_SCANNER_CONTRAST) (int contrast);

/**
 * Saves the plate dimensions to the INI file.
 *
 * @param plateNum The corresponding plate number.
 * @param left The left margin in inches.
 * @param top The top margin in inches.
 * @param right The width in inches.
 * @param bottom The height in inches.
 *
 * @return SC_SUCCESS if valid. SC_INVALID_VALUE if bad value.
 */
EXPORT int slConfigPlateFrame(unsigned plateNum, double left, double top,
		double right, double bottom);

typedef int (*SL_CONFIG_PLATE_FRAME) (unsigned, double x0, double y0, double x1,
		double y1);

/**
 * Scans an image for the specified dimensions. The image is in Windows BMP
 * format.
 *
 * @param dpi The dots per inch for the image.
 * @param left The left margin in inches.
 * @param top The top margin in inches.
 * @param right The width in inches.
 * @param bottom The height in inches.
 * @param filename The file name to save the bitmap to.
 *
 * @return SC_SUCCESS if valid. SC_INVALID_VALUE if bad value.
 */
EXPORT int slScanImage(unsigned dpi, double left, double top,
		double right, double bottom, char * filename);

typedef int (*SL_SCAN_IMAGE) (unsigned dpi, double x0, double y0,
		double x1, double y1, char * filename);

/**
 *
 * @param dpi
 * @param plateNum
 * @param filename
 * @return
 */
EXPORT int slScanPlate(unsigned dpi, unsigned plateNum, char * filename);

typedef int (*SL_SCAN_PLATE) (unsigned dpi, unsigned plateNum, char * filename);


/**
 *
 * @param dpi
 * @param plateNum
 * @return
 */
EXPORT int slCalibrateToPlate(unsigned dpi, unsigned plateNum);

typedef int (*SL_CALIBRATE_TO_PLATE) (unsigned dpi, unsigned plateNum);

/**
 *
 * @param dpi
 * @param plateNum
 * @return
 */
EXPORT int slDecodePlate(unsigned dpi, unsigned plateNum);

typedef int (*SL_DECODE_PLATE) (unsigned dpi, unsigned plateNum);

/**
 *
 * @param plateNum
 * @param filename
 * @return
 */
EXPORT int slDecodeImage(unsigned plateNum, char * filename);

typedef int (*SL_DECODE_IMAGE)(unsigned plateNum, char * filename);

#ifdef __cplusplus
}
#endif

#endif /* __INC_SCANLIB_H */
