/*
 * Util.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: nelson
 */

#include "Util.h"

#include <stdlib.h>

bool Util::strToNum(string & str, int & number, unsigned base) {
	/* Don't know for registerReset case */
	if (base == 10) {
		unsigned pos = str.find_first_not_of("0123456789");
		if (pos != string::npos) return false;
	}
	else if (base == 2) {
		unsigned pos = str.find_first_not_of("01");
		if (pos != string::npos) return false;
	}
	else if (base == 8) {
		unsigned pos = str.find_first_not_of("01234567");
		if (pos != string::npos) return false;
	}
	else if ((base == 16) || (base == 0)) {
		unsigned pos = str.find_first_not_of("0123456789abcdefABCDEF");
		if (pos != string::npos) return false;
	}

	number = strtoul(str.c_str (), NULL, base);
	return true;
}
