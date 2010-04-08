%module ScanLibWin32Wrapper
%{
extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, 
    int contrast, unsigned plateNum, double left, double top, double right,
    double bottom, unsigned scanGap, unsigned squareDev,
    unsigned edgeThresh);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        unsigned scanGap, unsigned squareDev, unsigned edgeThresh);
%}

extern int slIsTwainAvailable();
extern int slSelectSourceAsDefault();
extern int slScanImage(unsigned verbose, unsigned dpi, int brightness,
        int contrast, double left, double top, double right, double bottom,
        char * filename);
extern int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, 
    int contrast, unsigned plateNum, double left, double top, double right,
    double bottom, unsigned scanGap, unsigned squareDev,
    unsigned edgeThresh, unsigned corrections);
extern int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
        unsigned scanGap, unsigned squareDev, unsigned edgeThresh, 
        unsigned corrections);
        
