%module ScanLibWin32
%{
extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slConfigScannerBrightness(int brightness);
extern int slConfigScannerContrast(int contrast);
extern int slConfigPlateFrame(unsigned plateNum, double left, double top,
        double right, double bottom);
extern int slScanImage(unsigned dpi, double left, double top,
        double right, double bottom, char * filename);
extern int slScanPlate(unsigned dpi, unsigned plateNum, char * filename);
extern int slCalibrateToPlate(unsigned dpi, unsigned plateNum);
extern int slDecodePlate(unsigned dpi, unsigned plateNum);
extern int slDecodeImage(unsigned plateNum, char * filename);
%}


extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slConfigScannerBrightness(int brightness);
extern int slConfigScannerContrast(int contrast);
extern int slConfigPlateFrame(unsigned plateNum, double left, double top,
        double right, double bottom);
extern int slScanImage(unsigned dpi, double left, double top,
        double right, double bottom, char * filename);
extern int slScanPlate(unsigned dpi, unsigned plateNum, char * filename);
extern int slCalibrateToPlate(unsigned dpi, unsigned plateNum);
extern int slDecodePlate(unsigned dpi, unsigned plateNum);
extern int slDecodeImage(unsigned plateNum, char * filename);