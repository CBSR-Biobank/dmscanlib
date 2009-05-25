#ifndef __INC_ImageProcessor_h
#define __INC_ImageProcessor_h

#include "dmtx.h"

class Dib;
class Decoder;

class ImageProcessor {
public:
	ImageProcessor();
	~ImageProcessor();

	void decodeDib(char * filename);
	void decodeDib(Dib * image);
	void decodeImage(DmtxImage * image);

private:
	void debugTags(Decoder * decoder);

	DmtxImage* image;
};


#endif /* __INC_ImageProcessor_h */
