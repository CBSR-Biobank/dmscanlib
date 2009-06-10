/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
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
	ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
}

Decoder::~Decoder() {
}

void Decoder::processImage(Dib & dib, vector<MessageInfo *>  & msgInfos){
	DmtxImage * image = createDmtxImageFromDib(dib);
	processImage(*image, msgInfos);
	dmtxImageDestroy(&image);
}

void Decoder::processImage(DmtxImage & image, vector<MessageInfo *>  & msgInfos) {
	DmtxDecode * dec = NULL;
	DmtxRegion * reg = NULL;
	DmtxMessage * msg = NULL;
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
			FILE * fh;
			unsigned char *pnm;
			int totalBytes;
			int headerBytes;

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
			MessageInfo * info = new MessageInfo(dec, reg, msg);
			UA_ASSERT_NOT_NULL(info);

			//showStats(dec, reg, msg);
			msgInfos.push_back(info);
			UA_DOUT(1, 5, "message " << msgInfos.size() - 1
					<< ": "	<< msgInfos.back()->getMsg());
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
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

/*
 *	createDmtxImageFromDib
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

void Decoder::getRegionsFromIni(CSimpleIniA & ini) {
	const CSimpleIniA::TKeyVal * values = ini.GetSection(INI_SECTION_NAME);
	if (values == NULL) {
		cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "] not defined in ini file." << endl
			 << "Please run calibration first." << endl;
		exit(1);
	}
	if (values->size() == 0) {
		cerr << "INI file error: section [" << INI_SECTION_NAME
			     << "] does not define any regions." << endl
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

	vector<MessageInfo *>  msgInfos;

	for (unsigned i = 0, n = decodeRegions.size(); i < n; ++i) {
		DecodeRegion & region = *decodeRegions[i];
		Dib croppedDib;
		croppedDib.crop(dib, region.topLeft.X, region.topLeft.Y,
				region.botRight.X, region.botRight.Y);
		msgInfos.clear();
		processImage(croppedDib, msgInfos);
		unsigned size = msgInfos.size();
		UA_ASSERT(size <= 1);
		if (size == 1) {
			region.msgInfo = msgInfos[0];
		}
	}
}

ostream & operator<<(ostream &os, DecodeRegion & r) {
	os << r.row	<< "," << r.col << ": "
		<< "(" << r.topLeft.X << ", " << r.topLeft.Y << "), "
		<< "(" << r.botRight.X << ", " << r.botRight.Y << ")";
	return os;
}
