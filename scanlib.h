// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SCANLIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SCANLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SCANLIB_EXPORTS
#define SCANLIB_API __declspec(dllexport)
#else
#define SCANLIB_API __declspec(dllimport)
#endif

// This class is exported from the scanlib.dll
class SCANLIB_API Cscanlib {
public:
	Cscanlib(void);
	// TODO: add your methods here.
};

extern SCANLIB_API int nscanlib;

SCANLIB_API int fnscanlib(void);
