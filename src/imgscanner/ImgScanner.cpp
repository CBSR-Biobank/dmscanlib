#include "ImgScanner.h"
#include "ImgScannerImpl.h"
#include "ImgScannerSimulator.h"

#include <memory>

namespace dmscanlib {

ImgScanner::ImgScanner() {

}

ImgScanner::~ImgScanner() {

}

std::unique_ptr<ImgScanner> ImgScanner::create() {
#ifdef WIN32
	return unique_ptr<ImgScanner>(new ImgScannerImpl());
#else
	return std::unique_ptr<ImgScanner>(new imgscanner::ImgScannerSimulator());
#endif
}

} /* namespace */
