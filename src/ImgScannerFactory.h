#ifndef __INC_IMG_SCANNER_FACTORY_H
#define __INC_IMG_SCANNER_FACTORY_H

#include <memory>

#if !defined _VISUALC_
#   include <tr1/memory>
#endif

using namespace std;

class ImgScanner;

class ImgScannerFactory {
public:
	static std::tr1::shared_ptr<ImgScanner> getImgScanner();


private:
	ImgScannerFactory();
	~ImgScannerFactory();
};

#endif /* __INC_IMG_SCANNER_FACTORY_H */
