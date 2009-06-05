#ifndef UTIL_H_
#define UTIL_H_

/*
 * Util.h
 *
 *  Created on: Jun 4, 2009
 *      Author: nelson
 */

#include <string>

using namespace std;

class Util {
public:
	static bool strToNum(string & str, int & number, unsigned base = 0);

private:
};

#endif /* UTIL_H_ */
