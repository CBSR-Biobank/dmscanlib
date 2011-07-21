%module DmScanLibWin32Wrapper
%{
extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slGetScannerCapability();
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slScanFlatbed(unsigned verbose, unsigned dpi, int brightness,
		int contrast, const char * filename);
extern int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, 
    int contrast, unsigned plateNum, double left, double top, double right,
    double bottom, double scanGap, unsigned squareDev,
    unsigned edgeThresh, unsigned corrections, double cellDistance, 
    double gapX, double gapY,
    unsigned profileA, unsigned profileB, unsigned profileC, unsigned isVertical);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        double scanGap, unsigned squareDev, unsigned edgeThresh, 
        unsigned corrections, double cellDistance, double gapX, double gapY,
        unsigned profileA, unsigned profileB, unsigned profileC, unsigned isVertical);
%}

extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slGetScannerCapability();
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slScanFlatbed(unsigned verbose, unsigned dpi, int brightness,
		int contrast, const char * filename);
extern int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, 
    int contrast, unsigned plateNum, double left, double top, double right,
    double bottom, double scanGap, unsigned squareDev,
    unsigned edgeThresh, unsigned corrections, double cellDistance, 
	double gapX, double gapY,
        unsigned profileA, unsigned profileB, unsigned profileC, unsigned isVertical);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        double scanGap, unsigned squareDev, unsigned edgeThresh, 
        unsigned corrections, double cellDistance, double gapX, double gapY,
        unsigned profileA, unsigned profileB, unsigned profileC, unsigned isVertical);
        
