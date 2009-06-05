#ifndef DECODER_H_
#define DECODER_H_

/*
 * Decoder.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "dmtx.h"

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <vector>
#include <string>

using namespace std;

class Dib;
struct MessageInfo;
struct Cluster;
struct DecodeRegion;

class Decoder {
public:
	Decoder();
	Decoder(Dib & dib);
	Decoder(DmtxImage & image);
	virtual ~Decoder();

	void decodeImage(Dib & dib);
	void decodeImageRegions(Dib & dib);
	void decodeImage(DmtxImage & image);

	unsigned getNumTags();
	const char * getTag(unsigned tagNum);
	void getTagCorners(int tagNum, DmtxVector2 & p00, DmtxVector2 & p10,
			DmtxVector2 & p11, DmtxVector2 & p01);

	string getResults();
	void debugShowTags();
	void saveRegionsToIni(CSimpleIniA & ini);
	void getRegionsFromIni(CSimpleIniA & ini);

private:
	vector<MessageInfo *> calRegions;
	vector<DecodeRegion *> decodeRegions;
	vector<Cluster *>     rowClusters;
	vector<Cluster *>     colClusters;
	static const int      CLUSTER_THRESH = 15;
	static const char *   INI_SECTION_NAME;
	static const char *   INI_REGION_LABEL;

	void clearResults();
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void sortRegions(unsigned imageHeight, unsigned imageWidth);
};

#endif /* DECODER_H_ */
