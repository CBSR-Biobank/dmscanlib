#ifndef CONFIG_H_
#define CONFIG_H_

/*
 * Config.h
 *
 *  Created on: Jun 12, 2009
 *      Author: loyola
 */

#include "ScanLib.h"

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <vector>

using namespace std;

class BarcodeInfo;
class BinRegion;
struct DecodeRegion;

class Config {
public:
	Config(const char * inifilename);
	virtual ~Config();

	int getState() { return state; }

	void save();
	void parseFrames();
	bool getPlateFrame(unsigned plate, ScFrame ** fame);
	bool setRegions(unsigned plateNum, const vector<BinRegion *> & rowBinRegions,
			const vector<BinRegion *> & colBinRegions, unsigned maxCol);
	bool parseRegions(unsigned plateNum);
	bool savePlateFrame(unsigned short plateNum, double left,
			double top,	double right, double bottom);

	vector<DecodeRegion *> & getRegions() {
		return regions;
	}

	static const unsigned MAX_PLATES = 4;

private:
	bool parseFrame(unsigned frameNum);

	static const int STATE_OK = 0;
	static const int STATE_FILE_OPEN = -1;
	static const int STATE_ERROR_LOAD = -2;

	static const char * INI_PLATE_SECTION_NAME;
	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	const char * inifilename;
	CSimpleIniA ini;
	int state;

	vector<DecodeRegion *> regions;

	map<unsigned, ScFrame> plateFrames;
};

#endif /* CONFIG_H_ */
