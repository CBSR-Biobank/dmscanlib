#ifndef DMTXDECODEHELPER_H_
#define DMTXDECODEHELPER_H_

/*
 * DmtxDecodeHelper.h
 *
 *  Created on: 2012-10-30
 *      Author: loyola
 */

#include <dmtx.h>

namespace dmscanlib {

namespace decoder {

class DmtxDecodeHelper {
public:
	DmtxDecodeHelper(DmtxImage * dmtxImage, int scale);
	virtual ~DmtxDecodeHelper();

	unsigned setProperty(int prop, int value);

	DmtxDecode * getDecode() { return dec; }

private:
	DmtxDecode *dec;
};

} /* namespace decoder */

} /* namespace dmscanlib */
#endif /* DMTXDECODEHELPER_H_ */
