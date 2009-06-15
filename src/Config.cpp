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

	fstream inifile;
	inifile.open(inifilename, fstream::in);
	if (!inifile.is_open()) {
		state = STATE_FILE_OPEN;
		return;
	}

	SI_Error rc = ini.Load(inifile);
	if (rc < 0) {
		UA_DOUT(5, 3, "attempt to load ini file failed");
		state = STATE_ERROR_LOAD;
		return;
	}
	inifile.close();
}

Config::~Config() {
	save();
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


bool Config::setRegions(unsigned plateNum, const vector<BinRegion *> & rowBinRegions,
		const vector<BinRegion *> & colBinRegions, unsigned maxCol) {
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

	for (int r = rowBinRegions.size() - 1; r >= 0; --r) {
		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			key = INI_REGION_LABEL + to_string(rowBinRegions[r]->getRank())
				+ "_" + to_string(maxCol - colBinRegions[c]->getRank());
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
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		pos = key.find_first_of('_');
		if (pos == string::npos) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		string numStr = key.substr(labelSize, pos - labelSize);
		if (!Util::strToNum(numStr, region->row, 10)) {
			UA_DOUT(5, 3, "INI file error: section " << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		numStr = key.substr(pos + 1);
		if (!Util::strToNum(numStr, region->col, 10)) {
			UA_DOUT(5, 3, "INI file error: section [" << secName
			     << "], key name \"" << key << "\" is invalid."  << endl
			     << "Please run calibration again.");
			return false;
		}

		pos = value.find_first_of(',');
		numStr = value.substr(0, pos);
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

		regions.push_back(region);
		UA_DOUT(5, 3, "getRegionsFromIni: " << *region);
	}
	return true;
}

bool Config::savePlateFrame(unsigned short plateNum, double left,
		double top,	double right, double bottom) {
	if (state != STATE_OK) {
		UA_DOUT(5, 3, "savePlateFrame: invalid ini state: " << state);
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
