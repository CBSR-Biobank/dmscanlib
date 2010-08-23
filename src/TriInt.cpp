#include "TriInt.h"

TriInt::TriInt(unsigned int  a, unsigned int  b, unsigned int c){
	this->a = a;
	this->b = b;
	this->c = c;
}

bool TriInt::isSetBit(unsigned bit) {
	if (bit >= 0 && bit < 96) {
		if (bit < 32) {
			return ((this->a & (1 << bit)) != 0);

		} else if (bit < 64) {
			return ((this->b & (1 << (bit - 32))) != 0);

		} else { // bit < 96
			return ((this->c & (1 << (bit - 64))) != 0);
		}
	}
	return false;
}
