#ifndef __INC_IMG_SCANNER_FACTORY_H
#define __INC_IMG_SCANNER_FACTORY_H

#include <memory>

using namespace std;

class ImgScanner;

class ImgScannerFactory {
public:
	static auto_ptr<ImgScanner> getImgScanner();


private:
	ImgScannerFactory();
	~ImgScannerFactory();
};

#endif /* __INC_IMG_SCANNER_FACTORY_H */
