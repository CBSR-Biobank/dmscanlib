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

std::tr1::shared_ptr<ImgScanner> ImgScannerFactory::getImgScanner() {
#ifdef WIN32
	return std::tr1::shared_ptr<ImgScanner>(new ImgScannerImpl());
#else
	return std::tr1::shared_ptr<ImgScanner>(new ImgScannerSimulator());
#endif
}
