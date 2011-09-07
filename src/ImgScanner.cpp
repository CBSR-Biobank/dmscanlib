#include "ImgScanner.h"
#include "ImgScannerImpl.h"
#include "ImgScannerSimulator.h"

ImgScanner::ImgScanner() {

}

ImgScanner::~ImgScanner() {

}


std::tr1::shared_ptr<ImgScanner> ImgScanner::create() {
#ifdef WIN32
	return std::tr1::shared_ptr<ImgScanner>(new ImgScannerImpl());
#else
	return std::tr1::shared_ptr<ImgScanner>(new ImgScannerSimulator());
#endif
}
