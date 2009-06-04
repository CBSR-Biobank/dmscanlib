/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaDebug.h"
#include "Dib.h"

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

struct MessageInfo {
	string str;
	DmtxVector2 p00, p10, p11, p01;

	DmtxVector2 & getTopLeftCorner() {
		double minDist = numeric_limits<double>::max();
		unsigned min = 4;

		double dist[4] = {
				dmtxVector2Mag(&p00),
				dmtxVector2Mag(&p10),
				dmtxVector2Mag(&p11),
				dmtxVector2Mag(&p01),
		};

		for (unsigned i = 0; i < 4; ++i) {
			if (dist[i] < minDist) {
				minDist = dist[i];
				min = i;
			}
		}

		switch (min) {
			case 0: return p00;
			case 1: return p10;
			case 2: return p11;
			case 3: return p01;
		}
		UA_ASSERTS(false, "invalid value for min: " << min);
	}

	friend ostream & operator<<(ostream & os, MessageInfo & m);
};

ostream & operator<<(ostream &os, MessageInfo & m) {
	os << "\"" << m.str	<< "\" (" << m.p00.X << ", " << m.p00.Y << "), "
		<< "(" << m.p10.X << ", " << m.p10.Y << "), "
		<< "(" << m.p11.X << ", " << m.p11.Y << "), "
		<< "(" << m.p01.X << ", " << m.p01.Y << ")";
	return os;
}

struct Cluster {
	unsigned position;
	unsigned rank;

	Cluster(unsigned p) {
		position = p;
		rank = numeric_limits<int>::max();
	}

	friend ostream & operator<<(ostream & os, Cluster & r);
};

ostream & operator<<(ostream & os, Cluster & c) {
	os << "pos/" << c.position << " rank/" << c.rank;
	return os;
}

struct ClusterSort {
	bool operator()(Cluster* const& a, Cluster* const& b) {
		return (a->position < b->position);
	}
};

Decoder::Decoder() {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
}

Decoder::Decoder(Dib & dib) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	decodeImage(dib);
}

Decoder::Decoder(DmtxImage & image) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	decodeImage(image);
}

Decoder::~Decoder() {
	clearResults();
}

void Decoder::clearResults() {
	Cluster * c;

	while (results.size() > 0) {
		MessageInfo * info = results.back();
		results.pop_back();
		delete info;
	}
	while (rowClusters.size() > 0) {
		c = rowClusters.back();
		rowClusters.pop_back();
		delete c;
	}
	while (colClusters.size() > 0) {
		c = colClusters.back();
		colClusters.pop_back();
		delete c;
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
void Decoder::decodeImage(Dib & dib){
	DmtxImage * image = createDmtxImageFromDib(dib);
	decodeImage(*image);
	dmtxImageDestroy(&image);
}

void Decoder::decodeImage(DmtxImage & image) {
	if (results.size() > 0) {
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

	UA_DOUT(1, 3, "decodeImage: image width/" << width
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

		UA_DOUT(1, 3, "retrieving message from region " << regionCount++);
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			messageAdd(dec, reg, msg);
			UA_DOUT(1, 3, "message " << results.size() - 1
					<< ": "	<< results.back()->str);
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
	sortRegions(height, width);
}

void Decoder::messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
	MessageInfo * info = new MessageInfo;
	UA_ASSERT_NOT_NULL(info);
	info->str.append((char *) msg->output, msg->outputIdx);

	int height = dmtxDecodeGetProp(dec, DmtxPropHeight);
	info->p00.X = info->p00.Y = info->p10.Y = info->p01.X = 0.0;
	info->p10.X = info->p01.Y = info->p11.X = info->p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&info->p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p01, reg->fit2raw);

	info->p00.Y = height - 1 - info->p00.Y;
	info->p10.Y = height - 1 - info->p10.Y;
	info->p11.Y = height - 1 - info->p11.Y;
	info->p01.Y = height - 1 - info->p01.Y;

	results.push_back(info);
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
	return results.size();
}

const char * Decoder::getTag(unsigned tagNum) {
	UA_ASSERT(tagNum < results.size());
	return results[tagNum]->str.c_str();
}

void Decoder::getTagCorners(int tagNum, DmtxVector2 & p00, DmtxVector2 & p10,
		DmtxVector2 & p11, DmtxVector2 & p01) {
	MessageInfo & info = *results[tagNum];
	p00 = info.p00;
	p10 = info.p10;
	p11 = info.p11;
	p01 = info.p01;
}

void Decoder::debugShowTags() {
	unsigned numTags = results.size();
	UA_DOUT(1, 1, "debugTags: tags found: " << numTags);
	for (unsigned i = 0; i < numTags; ++i) {
		MessageInfo & info = *results[i];
		UA_DOUT(1, 1, "debugTags: tag " << i << ": " << info);
	}
}

void Decoder::sortRegions(unsigned imageHeight, unsigned imageWidth) {
	bool rowClusterFound;
	bool colClusterFound;

	for (unsigned i = 0, n = results.size(); i < n; ++i) {
		rowClusterFound = false;
		colClusterFound = false;

		DmtxVector2 & tagCorner = results[i]->getTopLeftCorner();
		UA_DOUT(1, 9, "tag " << i << " : corner/" << tagCorner.X << "," << tagCorner.Y);

		for (unsigned c = 0, cn = colClusters.size(); c < cn; ++c) {
			Cluster & cluster = *colClusters[c];
			int diff = (int) tagCorner.X - (int)cluster.position;
			UA_DOUT(1, 9, "col " << c << ": diff/" << diff);

			if (abs(diff) < CLUSTER_THRESH) {
				colClusterFound = true;
			}

			// update cluster position based on cumulative moving average
			cluster.position += (unsigned) ((double) diff / (double) (i+1));
			UA_DOUT(1, 9, "col update position " << cluster);
		}

		for (unsigned r = 0, rn = rowClusters.size(); r < rn; ++r) {
			Cluster & cluster = *rowClusters[r];
			int diff = (int) tagCorner.Y - (int)cluster.position;
			UA_DOUT(1, 9, "row " << r << ": diff/" << diff);

			if (abs(diff) < CLUSTER_THRESH) {
				rowClusterFound = true;
			}

			// update cluster position based on cumulative moving average
			cluster.position += (unsigned) ((double) diff / (double) (i+1));
			UA_DOUT(1, 9, "row update position " << cluster);
		}

		if (!colClusterFound) {
			Cluster * newCluster = new Cluster((unsigned) tagCorner.X);
			UA_ASSERT_NOT_NULL(newCluster);
			UA_DOUT(1, 9, "new col " << colClusters.size() << ": " << *newCluster);
			colClusters.push_back(newCluster);
		}

		if (!rowClusterFound) {
			Cluster * newCluster = new Cluster((unsigned) tagCorner.Y);
			UA_ASSERT_NOT_NULL(newCluster);
			UA_DOUT(1, 9, "new row " << rowClusters.size() << ": " << *newCluster);
			rowClusters.push_back(newCluster);
		}
	}

	sort(rowClusters.begin(), rowClusters.end(), ClusterSort());
	sort(colClusters.begin(), colClusters.end(), ClusterSort());

	for (unsigned i = 0, n = rowClusters.size(); i < n; ++ i) {
		Cluster & c = *rowClusters[i];
		c.rank = i;
		UA_DOUT(1, 1, "row cluster " << i << ": " << c);
	}

	for (unsigned i = 0, n = colClusters.size(); i < n; ++ i) {
		Cluster & c = *colClusters[i];
		c.rank = i;
		UA_DOUT(1, 1, "col cluster " << i << ": " << c);
	}
}
