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
 * @return SC_SUCCESS if available, and SC_INVALID_VALUE if unavailable.
 */
EXPORT int slIsTwainAvailable();

typedef int (*SL_IS_TWAIN_AVAILABLE) ();

/**
 * Creates a dialog box to allow the user to select the scanner to use by
 * default.
 *
 * @return SC_SUCCESS when selected by the user, and SC_INVALID_VALUE if
 * the user cancelled the selection dialog.
 */
EXPORT int slSelectSourceAsDefault();

typedef int (*SL_SELECT_SOURCE_AS_DEFAULT) ();

/**
 * Saves the brightness value to be used for scanning to the INI file.
 *
 * @param brightness a value between -1000 and 1000.
 *
 * @return SC_SUCCESS if successfully saved to the INI file.
 * SC_INVALID_VALUE if brightness bad value. SC_INI_FILE_ERROR if unable to
 * save the value to the INI file.
 */
EXPORT int slConfigScannerBrightness(int brightness);

typedef int (*SL_CONFIG_SCANNER_BRIGHTNESS) (int brightness);

/**
 * Saves the contrast value to be used for scanning to the INI file.
 *
 * @param contrast a value between -1000 and 1000.
 *
 * @return SC_SUCCESS if successfully saved to the INI file.
 * SC_INVALID_VALUE if brightness bad value. SC_INI_FILE_ERROR if unable to
 * save the value to the INI file.
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
 * @return SC_SUCCESS if successfully saved to the INI file.
 * SC_INI_FILE_ERROR if unable to save the value to the INI file.
 */
EXPORT int slConfigPlateFrame(unsigned plateNum, double left, double top,
		double right, double bottom);

typedef int (*SL_CONFIG_PLATE_FRAME) (unsigned, double x0, double y0, double x1,
		double y1);

/**
 * Scans an image for the specified dimensions. The image is in Windows BMP
 * format.
 *
 * @param dpi      The dots per inch for the image. Possible values are 200,
 *                 300, 400, 600.
 * @param left     The left margin in inches.
 * @param top      The top margin in inches.
 * @param right    The width in inches.
 * @param bottom   The height in inches.
 * @param filename The file name to save the bitmap to.
 *
 * @return SC_SUCCESS if valid. SC_FAIL unable to scan an image.
 */
EXPORT int slScanImage(unsigned dpi, double left, double top,
		double right, double bottom, char * filename);

typedef int (*SL_SCAN_IMAGE) (unsigned dpi, double x0, double y0,
		double x1, double y1, char * filename);

/**
 * Scans a plate image from the dimensions specified in the INI file. The image
 * is saved to a file in Windows BMP format.
 *
 * @param dpi      The dots per inch for the image. Possible values are 200,
 *                 300, 400, 600.
 * @param plateNum The plate number. Must be a number beteen 1 and 4.
 * @param filename The file name to use for the bitmap file.
 *
 * @return SC_INVALID_DPI if the DPI value is invalid. SC_INVALID_PLATE_NUM if
 * the plate number is invalid. SC_FAIL if an invalid file name is given or the
 * scanning process failed. SC_FILE_SAVE_ERROR if the file could not be saved.
 * SC_SUCCESS if the image was successfully scanned.
 */
EXPORT int slScanPlate(unsigned dpi, unsigned plateNum, char * filename);

typedef int (*SL_SCAN_PLATE) (unsigned dpi, unsigned plateNum, char * filename);


/**
 * From the dimensions specified in the INI file for the plate, scans an image
 * and then attempts to find all 2d barcodes in the image. The barcodes are then
 * grouped into rows and columns and the regions they fall in are saved to
 * the INI file.
 *
 * Calling this function also create the "calibrated.bmp" windows bitmap file
 * which shows the decode regions in rows and columns.
 *
 * @param dpi      The dots per inch for the image. Possible values are 200,
 *                 300, 400, 600.
 * @param plateNum The plate number. Must be a number beteen 1 and 4.
 *
 * @return SC_INVALID_DPI if the DPI value is invalid. SC_INVALID_PLATE_NUM if
 * the plate number is invalid. SC_INI_FILE_ERROR if the plate dimensions
 * cannot be found in the INI file or the results cannot be saved to the  INI
 * file. SC_FAIL the scanning process failed. SC_CALIBRATOR_NO_REGIONS if no
 * barcode regions are found in the image. SC_SUCCESS if the regions are found
 * and successfully saved to the INI file.
 */
EXPORT int slCalibrateToPlate(unsigned dpi, unsigned plateNum);

typedef int (*SL_CALIBRATE_TO_PLATE) (unsigned dpi, unsigned plateNum);

/**
 * From the regions specified in the INI file for the corresponding plate,
 * scans an image and then decodes all the regions. The decoded barcodes are
 * written to the file "scanlib.txt". The scanlib.txt file is a comma separated
 * value file with the following columns: Plate, Row, Column, Barcode.
 *
 * Calling this function also creates the "decoded.bmp" windows bitmap file.
 * This file shows a green square around the barcodes that were successfully
 * decoded. If the regions failed to decode then a red square is drawn around
 * it.
 *
 * @param dpi      The dots per inch for the image. Possible values are 200,
 *                 300, 400, 600.
 * @param plateNum The plate number. Must be a number beteen 1 and 4.
 *
 * @return SC_INI_FILE_ERROR if the regions are not found in the INI file.
 * SC_SUCCESS if the scanning process was successful.
 */
EXPORT int slDecodePlate(unsigned dpi, unsigned plateNum);

typedef int (*SL_DECODE_PLATE) (unsigned dpi, unsigned plateNum);

/**
 * From the regions specified in the INI file for the corresponding plate,
 * decodes all the regions. The decoded barcodes are written to the file
 * "scanlib.txt". The scanlib.txt file is a comma separated value file with the
 * following columns: Plate, Row, Column, Barcode.
 *
 * Calling this function also creates the "decoded.bmp" windows bitmap file.
 * This file shows a green square around the barcodes that were successfully
 * decoded. If the regions failed to decode then a red square is drawn around
 * it.
 *
 * @param plateNum The plate number. Must be a number beteen 1 and 4.
 * @param filename The windows bitmap file to decode.
 * @return
 */
EXPORT int slDecodeImage(unsigned plateNum, char * filename);

typedef int (*SL_DECODE_IMAGE)(unsigned plateNum, char * filename);

#ifdef __cplusplus
}
#endif

#endif /* __INC_SCANLIB_H */
