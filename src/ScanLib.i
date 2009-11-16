%module ScanLibWin32Wrapper
%{
extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slConfigPlateFrame(unsigned plateNum, double left, double top,
        double right, double bottom);
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slScanPlate(unsigned verbose, unsigned dpi, unsigned plateNum,
        int brightness, int contrast, char * filename);
extern int slCalibrateToPlate(unsigned dpi, unsigned plateNum);
extern int
slDecodePlate(unsigned verbose, unsigned dpi, unsigned plateNum,
        int brightness, int contrast, unsigned scanGap, unsigned squareDev,
        unsigned edgeThresh);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        unsigned scanGap, unsigned squareDev, unsigned edgeThresh);
%}

extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slConfigPlateFrame(unsigned plateNum, double left, double top,
        double right, double bottom);
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slScanPlate(unsigned verbose, unsigned dpi, unsigned plateNum,
        int brightness, int contrast, char * filename);
extern int slCalibrateToPlate(unsigned dpi, unsigned plateNum);
extern int
slDecodePlate(unsigned verbose, unsigned dpi, unsigned plateNum,
        int brightness, int contrast, unsigned scanGap, unsigned squareDev,
        unsigned edgeThresh);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        unsigned scanGap, unsigned squareDev, unsigned edgeThresh);
        
