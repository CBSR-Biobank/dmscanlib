#ifndef UTIL_H_
#define UTIL_H_

/*
 * Util.h
 *
 *  Created on: Jun 4, 2009
 *      Author: nelson
 */

#include <string>
#include <sstream>

using namespace std;

class Util {
public:
	static bool strToNum(string & str, int & number, unsigned base = 0);
	bool strToNum(string & str, double & number);

private:
};

template <typename T>
string to_string(T const& value) {
    stringstream sstr;
    sstr << value;
    return sstr.str();
}


#endif /* UTIL_H_ */
