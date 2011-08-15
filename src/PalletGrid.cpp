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
#include "PalletCell.h"
#include "Dib.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "cxtypes.h"

#include <sstream>

PalletGrid::PalletGrid(Orientation o, std::tr1::shared_ptr<const Dib> img,
		std::tr1::shared_ptr<Decoder> dcdr, unsigned gapX, unsigned gapY,
		const unsigned(&profileWords)[3]) :
		image(img), decoder(dcdr) {
	orientation = o;

	const int imgWidth = image->getWidth();
	const int imgHeight = image->getHeight();

	if (orientation == ORIENTATION_HORIZONTAL) {
		imgValid = imgWidth > imgHeight;
		cellWidth = imgWidth / static_cast<double>(MAX_COLS);
		cellHeight = imgHeight / static_cast<double>(MAX_ROWS);
	} else if (orientation == ORIENTATION_VERTICAL) {
		imgValid = imgWidth < imgHeight;
		cellWidth = imgWidth / static_cast<double>(MAX_ROWS);
		cellHeight = imgHeight / static_cast<double>(MAX_COLS);
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
	cellEnabled.resize(NUM_CELLS);
	unsigned mask;
	for (unsigned i = 0; i < NUM_CELLS / 32; ++i) {
		mask = 1;
		for (unsigned j = 0; j < 32; ++j) {
			if (profileWords[i] & mask) {
				cellEnabled[32 * i + j] = 1;
			}
			mask <<= 1;
		}
	}

	cellsByRowCol.resize(MAX_ROWS);

	CvRect rect;
	for (unsigned row = 0, rows = MAX_ROWS; row < rows; ++row) {
		cellsByRowCol[row].resize(MAX_COLS);
		for (unsigned col = 0, cols = MAX_COLS; col < cols; ++col) {
			if (!cellEnabled[MAX_COLS * row + col])
				return;

			getCellRect(row, col, rect);
			std::tr1::shared_ptr<Dib> cellImage = image->crop(rect.x + gapX,
					rect.y + gapY, rect.x + rect.width - gapX,
					rect.y + rect.height - gapY);
			std::tr1::shared_ptr<PalletCell> cell(
					new PalletCell(cellImage, row, col));
			cells.push_back(cell);
			cellsByRowCol[row][col] = cell;

		}
	}
}

PalletGrid::~PalletGrid() {
}

void PalletGrid::createImageWithCells() {

	imageWithCells = std::tr1::shared_ptr<Dib>(new Dib(*image));

	CvRect rect;
	for (unsigned row = 0, rows = MAX_ROWS; row < rows; ++row) {
		for (unsigned col = 0, cols = MAX_COLS; col < cols; ++col) {

		}
	}
}

void PalletGrid::getCellRect(unsigned row, unsigned col, CvRect & rect) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	if (orientation == ORIENTATION_HORIZONTAL) {
		rect.x = static_cast<int>(cellWidth * (MAX_COLS - col - 1)) + gapX;
		rect.y = static_cast<int>(cellHeight * row) + gapY;
	} else if (orientation == ORIENTATION_VERTICAL) {
		rect.x = static_cast<int>(cellWidth * row) + gapX;
		rect.y = static_cast<int>(cellHeight * col) + gapY;
	} else {
		UA_ASSERTS(false, "orientation invalid: " << orientation);
	}

	rect.width = static_cast<int>(cellWidth) - 2 * gapX;
	rect.height = static_cast<int>(cellHeight) - 2 * gapY;
}

void PalletGrid::getPositionStr(unsigned row, unsigned col, std::string & str) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	std::ostringstream out;
	out << (char) ('A' + row) << col;
	str = out.str();
}

bool PalletGrid::getCellEnabled(unsigned row, unsigned col) {
	UA_ASSERTS(row < PalletGrid::MAX_ROWS, "invalid row requested " << row);
	UA_ASSERTS(col < PalletGrid::MAX_COLS, "invalid column requested " << col);
	return cellEnabled[MAX_COLS * row + col];
}

std::tr1::shared_ptr<const Dib> PalletGrid::getCellImage(unsigned row,
		unsigned col) {
	UA_ASSERT(row < MAX_ROWS);
	UA_ASSERT(col < MAX_COLS);

	return cellsByRowCol[row][col]->getImage();
}

void PalletGrid::decodeCells() {
	UA_ASSERT(imgValid);

#ifdef _DEBUG
	string str;
	getProfileAsString(str);
	UA_DOUT(1, 5, "Profile: \n" << str);
#endif

	for (unsigned i, n = cells.size(); i < n; ++i) {

	}

}

void PalletGrid::getProfileAsString(string & str) {
	ostringstream out;
	for (unsigned row = 0; row < PalletGrid::MAX_ROWS; ++row) {
		for (unsigned col = 0; col < PalletGrid::MAX_COLS; ++col) {
			out << cellEnabled[MAX_COLS * row + col];
		}
		out << endl;
	}
	str = out.str();
}
