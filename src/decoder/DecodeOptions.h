#ifndef DECODEOPTIONS_H_
#define DECODEOPTIONS_H_

/*
 * DecodeOptions.h
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

#include <jni.h>
#include <ostream>
#include <memory>

namespace dmscanlib {

class Decoder;

class DecodeOptions {
public:
	DecodeOptions(long squareDev, long edgeThresh, long corrections,
			long shrink);
	virtual ~DecodeOptions();

	static std::unique_ptr<DecodeOptions> getDecodeOptionsViaJni(JNIEnv *env,
			jobject decodeOptionsObj);

private:
	friend class Decoder;
	friend std::ostream & operator<<(std::ostream & os,
			const DecodeOptions & m);

	const long squareDev;
	const long edgeThresh;
	const long corrections;
	const long shrink;
};

std::ostream & operator<<(std::ostream & os, const DecodeOptions & m);

} /* namespace */

#endif /* DECODEOPTIONS_H_ */
