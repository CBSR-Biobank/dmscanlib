#include "ImgScannerFactory.h"
#include "ImgScannerSimulator.h"
#include "UaAssert.h"

#ifdef WIN32
#   include "ImgScannerImpl.h"
#endif


ImgScannerFactory::ImgScannerFactory() {

}

ImgScannerFactory::~ImgScannerFactory() {

}

auto_ptr<ImgScanner> ImgScannerFactory::getImgScanner() {
#ifdef WIN32
	return auto_ptr<ImgScanner>(new ImgScannerImpl());
#else
	return auto_ptr<ImgScanner>(new ImgScannerSimulator());
#endif
}
