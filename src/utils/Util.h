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

#if defined (WIN32) && ! defined(__MINGW32__)
typedef time_t slTime;
#else
typedef struct timeval slTime;
#endif

class Util {
public:
	static bool strToNum(const char * str, int & number, unsigned base = 0);
	static bool strToNum(string & str, int & number, unsigned base = 0);
	static bool strToNum(string & str, double & number);
	static void getTime(slTime & tm);
	static void getTimestamp(std::string & str_r);
	static void difftiime(slTime & start, slTime & end, slTime & diff);

private:
};

template <typename T>
string to_string(T const& value) {
    stringstream sstr;
    sstr << value;
    return sstr.str();
}

ostream & operator<<(ostream &os, slTime & tm);

#endif /* UTIL_H_ */
