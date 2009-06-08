/*
 * Util.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: nelson
 */

#include "Util.h"

#include <stdlib.h>

bool Util::strToNum(string & str, int & number, unsigned base) {
	char  * end = 0;
	number = strtoul(str.c_str (), &end, base);
	return (*end == 0);
}

bool Util::strToNum(string & str, double & number) {
	char  * end = 0;
	number = strtod(str.c_str (), &end);
	return (*end == 0);

}
