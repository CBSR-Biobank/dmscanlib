/*
 * CellPosition.cpp
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes Libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PalletGrid.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "cxtypes.h"

#include <sstream>

const unsigned PalletGrid::MAX_ROWS = 8;
const unsigned PalletGrid::MAX_COLS = 12;
const unsigned PalletGrid::NUM_CELLS = PalletGrid::MAX_ROWS
		* PalletGrid::MAX_COLS;

PalletGrid::PalletGrid(Orientation o, unsigned imgWidth, unsigned imgHeight,
		unsigned gapX, unsigned gapY, const unsigned(&profileWords)[3]) {
	orientation = o;

	if (orientation == ORIENTATION_HORIZONTAL) {
		imgValid = imgWidth > imgHeight;
		cellWidth = imgWidth / MAX_COLS;
		cellHeight = imgHeight / MAX_ROWS;
	} else if (orientation == ORIENTATION_VERTICAL) {
		imgValid = imgWidth < imgHeight;
		cellWidth = imgWidth / MAX_ROWS;
		cellHeight = imgHeight / MAX_COLS;
	} else {
		UA_ASSERTS(false, "orientation invalid: " << orientation);
	}

	UA_DOUT(
			1,
			3,
			"PalletGrid: orientation/" << orientation << " width/" << imgWidth << " height/" << imgHeight << " imgValid/" << imgValid);

	if (!imgValid)
		return;

	this->gapX = gapX;
	this->gapY = gapY;

	// load the profile
	bits.resize(NUM_CELLS);
	unsigned mask;
	for (unsigned i = 0; i < NUM_CELLS / 32; ++i) {
		mask = 1;
		for (unsigned j = 0; j < 32; ++j) {
			if (profileWords[i] & mask) {
				bits[32 * i + j] = 1;
			}
			mask <<= 1;
		}
	}
}

PalletGrid::~PalletGrid() {
}

void PalletGrid::getImageCoordinates(unsigned row, unsigned col,
		CvRect & rect) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	if (orientation == ORIENTATION_HORIZONTAL) {
		rect.x = cellWidth * (MAX_COLS - col - 1) + gapX;
		rect.y = cellHeight * row + gapY;
	} else if (orientation == ORIENTATION_VERTICAL) {
		rect.x = cellWidth * row + gapX;
		rect.y = cellHeight * col + gapY;
	} else {
		UA_ASSERTS(false, "orientation invalid: " << orientation);
	}

	rect.width = cellWidth - 2 * gapX;
	rect.height = cellWidth - 2 * gapY;
}

void PalletGrid::getPositionStr(unsigned row, unsigned col, string & str) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	ostringstream out;
	out << (char) ('A' + row) << col;
	str = out.str();
}

bool PalletGrid::getCellEnabled(unsigned row, unsigned col) {
	UA_ASSERTS(row < PalletGrid::MAX_ROWS, "invalid row requested " << row);
	UA_ASSERTS(col < PalletGrid::MAX_COLS, "invalid column requested " << col);
	return bits[PalletGrid::MAX_COLS * row + col];
}
