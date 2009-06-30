#ifndef CONFIG_H_
#define CONFIG_H_

/*
 * Config.h
 *
 *  Created on: Jun 12, 2009
 *      Author: loyola
 */

#include "structs.h"
#include "SimpleIni.h"

#include <vector>

using namespace std;

class BarcodeInfo;
class BinRegion;
struct DecodeRegion;

struct RegionInfo {
	unsigned dpi;
	vector<DecodeRegion *> regions;
};

class Config {
public:
	Config(const char * inifilename);
	virtual ~Config();

	int getState() { return state; }

	void save();
	void parseFrames();
	bool setPlateFrame(unsigned plateNum, double left,
			double top,	double right, double bottom);
	bool setScannerBrightness(int brightness);
	int getScannerBrightness();
	bool setScannerContrast(int contrast);
	int getScannerContrast();
	bool getPlateFrame(unsigned plate, ScFrame ** fame);
	bool setRegions(unsigned plateNum, unsigned dpi,
			const vector<BinRegion *> & rowBinRegions,
			const vector<BinRegion *> & colBinRegions);
	bool parseRegions(unsigned plateNum);

	const vector<DecodeRegion *> & getRegions(unsigned plateNum, unsigned dpi) const;

	unsigned getPlateRegionDpi(unsigned plateNum);

	static const unsigned MAX_PLATES = 4;

private:
	bool parseFrame(unsigned frameNum);

	static const int STATE_OK = 0;
	static const int STATE_FILE_NOT_OPEN = -1;
	static const int STATE_ERROR_LOAD = -2;

	static const char * INI_PLATE_SECTION_NAME;
	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	const char * inifilename;
	CSimpleIniA ini;
	int state;
	unsigned dpi;

	map<unsigned, ScFrame> plateFrames;
	RegionInfo regionInfos[MAX_PLATES];
};

#endif /* CONFIG_H_ */
