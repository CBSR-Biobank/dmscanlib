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
#include "cxtypes.h"

#include <sstream>

PalletGrid::PalletGrid(Orientation o, unsigned imgWidth, unsigned imgHeight,
		unsigned gapX, unsigned gapY) {
	orientation = o;

	if (orientation == ORIENTATION_HORIZONTAL) {
		cellWidth = imgWidth / MAX_COLS;
		cellHeight = imgHeight / MAX_ROWS;
	} else if (orientation == ORIENTATION_VERTICAL) {
		cellWidth = imgWidth / MAX_ROWS;
		cellHeight = imgHeight / MAX_COLS;
	} else {
		UA_ASSERTS(false, "orientation invalid: " << orientation);
	}

	this->gapX = gapX;
	this->gapY = gapY;
}

PalletGrid::~PalletGrid() {
}

void PalletGrid::getImageCoordinates(unsigned row, unsigned col, CvRect & rect) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	if (orientation == ORIENTATION_HORIZONTAL) {
		rect.x = cellWidth * (MAX_COLS - col - 1) + gapX;
		rect.y = cellHeight * (MAX_ROWS - row - 1) + gapY;
	} else if (orientation == ORIENTATION_VERTICAL) {
		rect.x = cellHeight * (MAX_COLS - col - 1) + gapY;
		rect.y = cellWidth * (MAX_ROWS - row - 1) + gapX;
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
