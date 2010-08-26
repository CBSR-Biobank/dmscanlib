/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
