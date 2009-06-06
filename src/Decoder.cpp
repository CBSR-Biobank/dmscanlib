/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaDebug.h"
#include "Dib.h"
#include "Util.h"
#include "MessageInfo.h"
#include "BinRegion.h"

#include <iostream>
#include <string.h>
#include <math.h>
#include <string>
#include <sstream>
#include <limits>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

using namespace std;

const char * Decoder::INI_SECTION_NAME = "barcode-regions";

const char * Decoder::INI_REGION_LABEL = "region";

Decoder::Decoder() {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
}

Decoder::Decoder(Dib & dib) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	processDib(dib);
}

Decoder::Decoder(DmtxImage & image) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	processImage(image);
}

Decoder::~Decoder() {
	clearResults();
}

void Decoder::clearResults() {
	while (calRegions.size() > 0) {
		MessageInfo * info = calRegions.back();
		calRegions.pop_back();
		delete info;
	}
}

/*
 *	decodeDib
 *	@params - filename: char* corresponding to the filename of an image
 *	@return - The number of bytes that got truncated to fit the barcodes
 *			  in the supplied buffer.
 *
 *	Create a file from the filename given, then create a DmtxImage from this
 *	file. If a DmxtImage can be created, decode it. All barcodes decoded are
 *	stored in the supplied buffer, up to a max length of bufferSize.
 */
void Decoder::processDib(Dib & dib){
	DmtxImage * image = createDmtxImageFromDib(dib);
	processImage(*image);
	dmtxImageDestroy(&image);
}

void Decoder::processImage(DmtxImage & image) {
	if (calRegions.size() > 0) {
		// an image was already created, destroy this one as a new one
		// is created below
		clearResults();
	}

	DmtxDecode * dec = NULL;
	DmtxRegion * reg = NULL;
	DmtxMessage * msg = NULL;
	FILE * fh;
	unsigned char *pnm;
	int totalBytes, headerBytes;
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);

	UA_DOUT(1, 3, "processImage: image width/" << width
			<< " image height/" << height
			<< " row padding/" << dmtxImageGetProp(&image, DmtxPropRowPadBytes)
			<< " image bits per pixel/"
			<< dmtxImageGetProp(&image, DmtxPropBitsPerPixel)
			<< " image row size bytes/"
			<< dmtxImageGetProp(&image, DmtxPropRowSizeBytes));

	dec = dmtxDecodeCreate(&image, 1);
	assert(dec != NULL);

	// save image to a PNM file
	UA_DEBUG(
			pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
			fh = fopen("out.pnm", "w");
			fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
			fclose(fh);
			free(pnm);
	);

	dmtxDecodeSetProp(dec, DmtxPropScanGap, 0);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, 10);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, 37);

	unsigned regionCount = 0;
	while (1) {
		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL) break;

		UA_DOUT(1, 5, "retrieving message from region " << regionCount++);
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			messageAdd(dec, reg, msg);
			UA_DOUT(1, 5, "message " << calRegions.size() - 1
					<< ": "	<< calRegions.back()->getMsg());
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
}

void Decoder::messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
	MessageInfo * info = new MessageInfo(dec, reg, msg);
	UA_ASSERT_NOT_NULL(info);

	//showStats(dec, reg, msg);
	calRegions.push_back(info);
}

void Decoder::showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
	int height;
	int dataWordLength;
	int rotateInt;
	double rotate;
	DmtxVector2 p00, p10, p11, p01;

	height = dmtxDecodeGetProp(dec, DmtxPropHeight);

	p00.X = p00.Y = p10.Y = p01.X = 0.0;
	p10.X = p01.Y = p11.X = p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

	dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, reg->sizeIdx);

	rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1]) -
			atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int)(rotate * 180/M_PI + 0.5);
	if(rotateInt >= 360)
		rotateInt -= 360;

	fprintf(stdout, "--------------------------------------------------\n");
	fprintf(stdout, "       Matrix Size: %d x %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
	fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
			dataWordLength - msg->padCount, dataWordLength);
	fprintf(stdout, "   Error Codewords: %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, reg->sizeIdx));
	fprintf(stdout, "      Data Regions: %d x %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx),
			dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
	fprintf(stdout, "Interleaved Blocks: %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, reg->sizeIdx));
	fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
	fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X, height - 1 - p00.Y);
	fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X, height - 1 - p10.Y);
	fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X, height - 1 - p11.Y);
	fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X, height - 1 - p01.Y);
	fprintf(stdout, "--------------------------------------------------\n");
}

/**
 *	createDmtxImageFromFile
 *
 *	Open the file and create a DmtxImage out of it.
 *
 *	@params - filename: char* corresponding to the filename of an image
 *			  dib - a blank Divice Independant Bitmap to read the image into
 *	@return - DmtxImage: the newly created image from the file.
 *
 */
DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib) {
	int pack = DmtxPack24bppRGB;

	if (dib.getBitsPerPixel() == 32) {
		pack = DmtxPack32bppXRGB;
	}

	// create dmtxImage from the dib
	DmtxImage * image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
			dib.getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dib.getRowPadBytes());
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return image;
}

unsigned Decoder::getNumTags() {
	return calRegions.size();
}

const char * Decoder::getTag(unsigned tagNum) {
	UA_ASSERT(tagNum < calRegions.size());
	return calRegions[tagNum]->getMsg().c_str();
}

void Decoder::getTagBoundingBox(int tagNum, DmtxVector2 & p00, DmtxVector2 & p10,
		DmtxVector2 & p11, DmtxVector2 & p01) {
	MessageInfo & info = *calRegions[tagNum];
	info.getCorners(p00, p10, p11, p01);
}

void Decoder::debugShowTags() {
	unsigned numTags = calRegions.size();
	UA_DOUT(1, 1, "debugTags: tags found: " << numTags);
	for (unsigned i = 0; i < numTags; ++i) {
		MessageInfo & info = *calRegions[i];
		UA_DOUT(1, 1, "debugTags: tag " << i << ": " << info);
	}
}

string Decoder::getResults() {
	ostringstream out;
	unsigned curRow = 0;

	for (unsigned i = 0, numTags = calRegions.size(); i < numTags; ++i) {
		MessageInfo & info = *calRegions[i];
		if (info.getRowBinRegion().getRank() != curRow) {
			out << endl;
			curRow = info.getRowBinRegion().getRank();
		}
		out << info.getMsg() << " ";
	}
	out << endl;
	return out.str();
}

void Decoder::getRegionsFromIni(CSimpleIniA & ini) {
	const CSimpleIniA::TKeyVal * values = ini.GetSection(INI_SECTION_NAME);
	if (values == NULL) {
		cerr << "INI file error: section [barcode-regions] not defined in ini file." << endl
			 << "Please run calibration first." << endl;
		exit(1);
	}
	if (values->size() == 0) {
		cerr << "INI file error: section [barcode-regions] does not define any regions." << endl
		     << "Please run calibration again." << endl;
		exit(1);
	}

	string label(INI_REGION_LABEL);
	unsigned labelSize = label.size(), pos, prevPos;
	DecodeRegion * region;

	for(CSimpleIniA::TKeyVal::const_iterator it = values->begin();
		it != values->end(); it++) {
		string key(it->first.pItem);
		string value(it->second);

		region = new DecodeRegion;
		UA_ASSERT_NOT_NULL(region);

		pos =  key.find(label);
		if (pos == string::npos) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		pos = key.find_first_of('_');
		if (pos == string::npos) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		string numStr = key.substr(labelSize, pos - labelSize);
		if (!Util::strToNum(numStr, region->row, 10)) {
			cerr << "INI file error: section " << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		numStr = key.substr(pos + 1);
		if (!Util::strToNum(numStr, region->col, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		pos = value.find_first_of(',');
		numStr = value.substr(0, pos);
		if (!Util::strToNum(numStr, region->topLeft.X, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], first value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->topLeft.Y, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], second value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->botRight.X, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], third value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		numStr = value.substr(pos + 1);
		if (!Util::strToNum(numStr, region->botRight.Y, 10)) {
			cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "], fourth value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again." << endl;
			exit(1);
		}

		decodeRegions.push_back(region);
		UA_DOUT(1, 3, "getRegionsFromIni: " << *region);
	}
}

/*
 * Should only be called after regions are loaded from INI file.
 */
void Decoder::processImageRegions(Dib & dib) {
	if (decodeRegions.size() == 0) {
		UA_WARN("no decoded regions; exiting.");
		return;
	}

	for (unsigned i = 0, n = decodeRegions.size(); i < n; ++i) {
		DecodeRegion & region = *decodeRegions[i];
		Dib croppedDib;
		croppedDib.crop(dib, region.topLeft.X, region.topLeft.Y,
				region.botRight.X, region.botRight.Y);
		processDib(croppedDib);
	}
}


ostream & operator<<(ostream &os, DecodeRegion & r) {
	os << r.row	<< "," << r.col << ": "
		<< "(" << r.topLeft.X << ", " << r.topLeft.Y << "), "
		<< "(" << r.botRight.X << ", " << r.botRight.Y << ")";
	return os;
}
