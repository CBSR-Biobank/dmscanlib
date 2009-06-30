/*
 * Config.cpp
 *
 *  Created on: Jun 12, 2009
 *      Author: loyola
 */

#include "Config.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"
#include "Util.h"


#ifdef WIN32
#include "ImageGrabber.h"
#endif

#include <iostream>
#include <fstream>

const char * Config::INI_PLATE_SECTION_NAME = "plate";
const char * Config::INI_SECTION_NAME = "barcode-regions";
const char * Config::INI_REGION_LABEL = "region";


Config::Config(const char * filename) :
	inifilename(filename), ini(true, false, true), state(STATE_OK) {
	ua::Logger::Instance().subSysHeaderSet(5, "CONFIG");

	SI_Error rc = ini.LoadFile(inifilename);
	if (rc < 0) {
		UA_DOUT(5, 1, "attempt to load ini file failed");
		state = STATE_ERROR_LOAD;
		return;
	}
}

Config::~Config() {
	save();

	for (unsigned i = 0; i < MAX_PLATES; ++ i) {
		for (unsigned j = 0, n = regionInfos[i].regions.size(); j < n; ++j) {
			if (regionInfos[i].regions[j]->msgInfo != NULL) {
				delete regionInfos[i].regions[j]->msgInfo;
			}
			delete regionInfos[i].regions[j];
		}
	}
}

void Config::save() {
	if (state != STATE_OK) {
		UA_DOUT(5, 3, "save: invalid ini state: " << state);
		return;
	}
	ini.SaveFile(inifilename);
}


void Config::parseFrames() {
	if (state != STATE_OK) {
		UA_DOUT(5, 3, "parseFrames: invalid ini state: " << state);
		return;
	}

	for (unsigned i = 1; i <= MAX_PLATES; ++i) {
		if (!parseFrame(i)) continue;

		UA_DOUT(5, 3, "plate " << i << ": top/" << plateFrames[i].y0
				<< " left/" << plateFrames[i].x0
				<< " bottom/" << plateFrames[i].y1
				<< " right/" << plateFrames[i].x0);
	}
}

bool Config::parseFrame(unsigned frameNum) {
	string secName(INI_PLATE_SECTION_NAME);
	secName += "-" + to_string(frameNum);

	const CSimpleIniA::TKeyVal * values = ini.GetSection(secName.c_str());
	if (values == NULL) return false;

	if (values->size() == 0) {
		UA_DOUT(5, 3, "INI file error: section [" << secName
		        << "], has no values");
		return false;
	}

	ScFrame frame;
	frame.frameId = frameNum;

	for(CSimpleIniA::TKeyVal::const_iterator it = values->begin();
		it != values->end(); it++) {
		string key(it->first.pItem);
		string value(it->second);

		if (key == "top") {
			if (!Util::strToNum(value, frame.y0)) {
				UA_DOUT(5, 3, "INI file error: section [" << secName
				        << "], value for key \""
					    << key << "\" is invalid: " << value);
				return false;
			}
		}
		else if (key == "left") {
			if (!Util::strToNum(value, frame.x0)) {
				UA_DOUT(5, 3, "INI file error: section [" << secName
				        << "], value for key \""
					    << key << "\" is invalid: " << value);
				return false;
			}
		}
		else if (key == "bottom") {
			if (!Util::strToNum(value, frame.y1)) {
				UA_DOUT(5, 3, "INI file error: section [" << secName
				        << "], value for key \""
					    << key << "\" is invalid: " << value);
				return false;
			}
		}
		else if (key == "right") {
			if (!Util::strToNum(value, frame.x1)) {
				UA_DOUT(5, 3, "INI file error: section [" << secName
				        << "], value for key \""
					    << key << "\" is invalid: " << value);
				return false;
			}
		}
		else {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			        << "], key is invalid: " << key);
			return false;
		}
	}
	plateFrames[frameNum] = frame;
	return true;
}

bool Config::setPlateFrame(unsigned plateNum, double left,
		double top,	double right, double bottom) {
	UA_ASSERTS((plateNum >0) && (plateNum <= MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);

	if (state != STATE_OK) {
		UA_DOUT(5, 3, "setPlateFrame: invalid ini state: " << state);
		return false;
	}

	SI_Error rc;

	string secname = "plate-" + to_string(plateNum);

	rc = ini.SetValue(secname.c_str(), "left", to_string(left).c_str());
	if (rc < 0) return false;

	rc = ini.SetValue(secname.c_str(), "top", to_string(top).c_str());
	if (rc < 0) return false;

	rc = ini.SetValue(secname.c_str(), "right", to_string(right).c_str());
	if (rc < 0) return false;

	rc = ini.SetValue(secname.c_str(), "bottom", to_string(bottom).c_str());
	if (rc < 0) return false;

	return true;
}

bool Config::setScannerBrightness(int brightness) {
	UA_ASSERTS((brightness >= -1000) && (brightness <= 1000),
			"setScannerBrightness: invalid brightness: " << brightness);

	if (state != STATE_OK) {
		UA_DOUT(5, 3, "setPlateFrame: invalid ini state: " << state);
		return false;
	}

	SI_Error rc;

	rc = ini.SetValue("scanner", "brightness", to_string(brightness).c_str());
	return (rc >= 0);
}

int Config::getScannerBrightness() {
	return static_cast<int>(ini.GetLongValue("scanner", "brightness", 0));
}

bool Config::setScannerContrast(int contrast) {
	UA_ASSERTS((contrast >= -1000) && (contrast <= 1000),
			"setScannerContrast: invalid contrast: " << contrast);

	if (state != STATE_OK) {
		UA_DOUT(5, 3, "setPlateFrame: invalid ini state: " << state);
		return false;
	}

	SI_Error rc;

	rc = ini.SetValue("scanner", "contrast", to_string(contrast).c_str());
	return (rc >= 0);
}

int Config::getScannerContrast() {
	return static_cast<int>(ini.GetLongValue("scanner", "contrast", 0));
}

bool Config::getPlateFrame(unsigned plate, ScFrame ** frame) {
	if (state != STATE_OK) {
		UA_DOUT(5, 3, "getPlateFrame: invalid ini state: " << state);
		return false;
	}

	map<unsigned, ScFrame>::iterator it = plateFrames.find(plate);
	if (it == plateFrames.end()) {
		*frame = NULL;
		UA_DOUT(5, 1,  "plate number " << plate << " is not defined in INI file.");
		return false;
	}
	*frame = &it->second;
	return true;
}


bool Config::setRegions(unsigned plateNum, unsigned dpi,
		const vector<BinRegion *> & rowBinRegions,
		const vector<BinRegion *> & colBinRegions) {
	UA_ASSERTS((plateNum >0) && (plateNum <= MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);

	if (state != STATE_OK) {
		UA_DOUT(5, 3, "setRegions: invalid ini state: " << state);
		return false;
	}

	if ((rowBinRegions.size() == 0) || (colBinRegions.size() == 0)) {
		UA_DOUT(5, 3, "setRegions: no regions found");
		return false;
	}

	UA_DOUT(5, 3, "setRegions: rows/" << rowBinRegions.size() << " cols/"
			<< colBinRegions.size());

	SI_Error rc;
	string key, value;
	string secName("plate-" + to_string(plateNum) + "-" + INI_SECTION_NAME);

	key = INI_REGION_LABEL;
	key += "_DPI";
	value = to_string(dpi);
	rc = ini.SetValue(secName.c_str(), key.c_str(), value.c_str());
	if (rc < 0) {
		UA_DOUT(5, 3, "setRegions: ini SetValue() returned: " << rc);
		return false;
	}

	for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			key = INI_REGION_LABEL;
			key += '_';
			key += static_cast<char>('A' + rowBinRegions[r]->getRank());
			key += "_" + to_string(cn - colBinRegions[c]->getRank());
			value = to_string(colBinRegions[c]->getMin()) + ","
				+ to_string(rowBinRegions[r]->getMin()) + ","
				+ to_string(colBinRegions[c]->getMax()) + ","
				+ to_string(rowBinRegions[r]->getMax());

			rc = ini.SetValue(secName.c_str(), key.c_str(), value.c_str());
			if (rc < 0) {
				UA_DOUT(5, 3, "setRegions: ini SetValue() returned: " << rc);
				return false;
			}
		}
	}
	return true;
}

bool Config::parseRegions(unsigned plateNum) {
	UA_ASSERTS((plateNum > 0) && (plateNum <= MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);

	if (state != STATE_OK) {
		UA_DOUT(5, 3, "parseRegions: invalid ini state: " << state);
		return false;
	}

	string secName = "plate-" + to_string(plateNum) + "-" + INI_SECTION_NAME;

	const CSimpleIniA::TKeyVal * values = ini.GetSection(secName.c_str());
	if (values == NULL) {
		UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "] not defined in ini file." << endl
			 << "Please run calibration first.");
		exit(1);
	}
	if (values->size() == 0) {
		UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "] does not define any regions." << endl
		     << "Please run calibration again.");
		return false;
	}

	DecodeRegion * region;
	string label(INI_REGION_LABEL);
	string dpiLabel(INI_REGION_LABEL);

	label += '_';
	dpiLabel += "_DPI";
	unsigned labelSize = label.size(), pos, prevPos;

	--plateNum;
	for(CSimpleIniA::TKeyVal::const_iterator it = values->begin();
		it != values->end(); it++) {
		string key(it->first.pItem);
		string value(it->second);

		if (key.find(dpiLabel) == 0) {
			int num;
			if (!Util::strToNum(value, num, 10)) {
				UA_DOUT(5, 3, "INI file error: section [" << secName
				     << "], key name \"" << key << "\" is invalid."  << endl
				     << "Please run calibration again.");
				return false;
			}
			regionInfos[plateNum].dpi = static_cast<unsigned>(num);
			continue;
		}

		region = new DecodeRegion;
		UA_ASSERT_NOT_NULL(region);

		pos =  key.find(label);
		if (pos == string::npos) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		pos = key.find_first_of('_', labelSize);
		if (pos == string::npos) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		string rowStr = key.substr(labelSize, pos - labelSize);
		if (rowStr.size() != 1) {
			UA_DOUT(5, 3, "INI file error: section " << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}
		region->row = static_cast<unsigned>(rowStr[0] - 'A');

		string colStr = key.substr(pos + 1);
		if (!Util::strToNum(colStr, region->col, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}
		--region->col;

		pos = value.find_first_of(',');
		string numStr = value.substr(0, pos);
		if (!Util::strToNum(numStr, region->topLeft.X, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], first value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again.");
			return false;
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->topLeft.Y, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], second value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again.");
			return false;
		}

		prevPos = pos + 1;
		pos = value.find_first_of(',', prevPos);
		numStr = value.substr(prevPos, pos - prevPos);
		if (!Util::strToNum(numStr, region->botRight.X, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], third value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again.");
			return false;
		}

		numStr = value.substr(pos + 1);
		if (!Util::strToNum(numStr, region->botRight.Y, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], fourth value for key \""
				 << key << "\" is invalid:" << numStr << endl
			     << "Please run calibration again.");
			return false;
		}

		regionInfos[plateNum].regions.push_back(region);
		UA_DOUT(5, 3, "getRegionsFromIni: " << *region);
	}
	return true;
}

void Config::setDecodeResults(unsigned plateNum) {
	UA_ASSERTS((plateNum >0) && (plateNum <= MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);
	ofstream file;
	file.open("scanlib.txt", ios::out);

	file << "#Plate,Row,Col,Barcode" << endl;

	for (unsigned i = 0, n = regionInfos[plateNum].regions.size(); i < n; ++i) {
		DecodeRegion & region = *regionInfos[plateNum].regions[i];

		if (region.msgInfo == NULL) continue;

		file << to_string(plateNum) << ","
		     << static_cast<char>('A' + region.row) << ","
		     << to_string(region.col + 1) << ","
		     << region.msgInfo->getMsg() << endl;
	}
	file.close();
}

const vector<DecodeRegion *> & Config::getRegions(unsigned plateNum, unsigned dpi) const {
	UA_ASSERTS((plateNum >0) && (plateNum <= MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);

	--plateNum;
	if (regionInfos[plateNum].dpi != dpi) {
		// convert regions to new dpi
		double factor = static_cast<double>(dpi)
		/ static_cast<double>( regionInfos[plateNum].dpi);

		const vector<DecodeRegion *> & regions = regionInfos[plateNum].regions;

		for (unsigned i = 0, n = regions.size(); i < n; ++i) {
			DecodeRegion & region = *regions[i];
			region.topLeft.X = static_cast<unsigned>(region.topLeft.X * factor);
			region.topLeft.Y = static_cast<unsigned>(region.topLeft.Y * factor);
			region.botRight.X = static_cast<unsigned>(region.botRight.X * factor);
			region.botRight.Y = static_cast<unsigned>(region.botRight.Y * factor);
		}
	}
	return regionInfos[plateNum].regions;
}
