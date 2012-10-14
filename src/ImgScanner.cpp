#include "ImgScanner.h"
#include "ImgScannerImpl.h"
#include "ImgScannerSimulator.h"

#include <memory>

ImgScanner::ImgScanner() {

}

ImgScanner::~ImgScanner() {

}


unique_ptr<ImgScanner> ImgScanner::create() {
#ifdef WIN32
	return unique_ptr<ImgScanner>(new ImgScannerImpl());
#else
	return unique_ptr<ImgScanner>(new ImgScannerSimulator());
#endif
}
