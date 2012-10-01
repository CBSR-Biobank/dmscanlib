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

#include "DmScanLibInternal.h"
#include "PalletGrid.h"
#include "PalletThreadMgr.h"
#include "PalletCell.h"
#include "Dib.h"
#include "geometry.h"

#include <glog/logging.h>
#include <sstream>

PalletGrid::PalletGrid(unsigned pn, Orientation o,
		std::tr1::shared_ptr<const Dib> img, unsigned gapX, unsigned gapY,
		const unsigned(&profileWords)[3]) :
		plateNum(pn), orientation(o), image(img) {

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
		CHECK(false) << "orientation invalid: " << orientation;
	}

	GCC_EXT VLOG(2)
			<< "PalletGrid: orientation/" << orientation << " width/"
					<< imgWidth << " height/" << imgHeight << " imgValid/"
					<< imgValid;

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

	// initialize enabled cells
	Rect rect;
	cellsByRowCol.resize(MAX_ROWS);
	for (unsigned row = 0, rows = MAX_ROWS; row < rows; ++row) {
		cellsByRowCol[row].resize(MAX_COLS);
		for (unsigned col = 0, cols = MAX_COLS; col < cols; ++col) {
			if (!cellEnabled[MAX_COLS * row + col])
				return;

			getCellRect(row, col, rect);
			std::tr1::shared_ptr<PalletCell> cell(
					new PalletCell(*this, row, col, rect));
			enabledCells.push_back(cell);
			cellsByRowCol[row][col] = cell;
		}
	}
}

PalletGrid::~PalletGrid() {
}

void PalletGrid::applyFilters() {
	if (image->getBitsPerPixel() != 8) {
		filteredImage = image->convertGrayscale();
	} else {
		filteredImage = std::tr1::shared_ptr<Dib>(new Dib(*image));
	}
	filteredImage->tpPresetFilter();
	if (GCC_EXT VLOG_IS_ON(2)) {
		image->writeToFile("filtered.bmp");
	}
}

void PalletGrid::getCellRect(unsigned row, unsigned col, Rect & rect) {
	CHECK(row < MAX_ROWS);
	CHECK(col < MAX_COLS);

	if (orientation == ORIENTATION_HORIZONTAL) {
		rect.x = static_cast<int>(cellWidth * (MAX_COLS - col - 1)) + gapX;
		rect.y = static_cast<int>(cellHeight * row) + gapY;
	} else if (orientation == ORIENTATION_VERTICAL) {
		rect.x = static_cast<int>(cellWidth * row) + gapX;
		rect.y = static_cast<int>(cellHeight * col) + gapY;
	} else {
		CHECK(false) << "orientation invalid: " << orientation;
	}

	rect.width = static_cast<int>(cellWidth) - gapX;
	rect.height = static_cast<int>(cellHeight) - gapY;
}

void PalletGrid::getPositionStr(unsigned row, unsigned col, std::string & str) {
	CHECK(row < MAX_ROWS);
	CHECK(col < MAX_COLS);

	std::ostringstream out;
	out << (char) ('A' + row) << col;
	str = out.str();
}

bool PalletGrid::getCellEnabled(unsigned row, unsigned col) {
	CHECK(row < PalletGrid::MAX_ROWS) << "invalid row requested " << row;
	CHECK(col < PalletGrid::MAX_COLS) << "invalid column requested " << col;
	return cellEnabled[MAX_COLS * row + col];
}

std::tr1::shared_ptr<const Dib> PalletGrid::getCellImage(unsigned row,
		unsigned col) {
	CHECK(row < MAX_ROWS);
	CHECK(col < MAX_COLS);

	return cellsByRowCol[row][col]->getImage();
}

std::tr1::shared_ptr<const Dib> PalletGrid::getCellImage(
		const PalletCell & cell) {
	cellsByRowCol.resize(MAX_ROWS);

	const unsigned row = cell.getRow();
	const unsigned col = cell.getCol();

	CHECK(cellEnabled[MAX_COLS * row + col]);

	const Rect & rect = cell.getParentPos();

	return filteredImage->crop(rect.x, rect.y, rect.x + rect.width,
			rect.y + rect.height);
}

unsigned PalletGrid::decodeCells(std::tr1::shared_ptr<Decoder> dcdr) {
	CHECK(imgValid);

	decoder = dcdr;

	if (GCC_EXT VLOG_IS_ON(3)) {
		string str;
		getProfileAsString(str);
		GCC_EXT VLOG(3) << "Profile: \n" << str;
		writeImageWithCells("cellRegions.bmp");
	}

	bool found;
	PalletThreadMgr imageManager(decoder);
	imageManager.decodeCells(enabledCells);

	for (unsigned i = 0, n = enabledCells.size(); i < n; ++i) {
		PalletCell & cell = *enabledCells[i];

		found = (cell.getDecodeValid() && !cell.getBarcodeMsg().empty());
		if (found) {
			decodedCells.push_back(enabledCells[i]);
		}

		if (GCC_EXT VLOG_IS_ON(2)) {
			cell.writeImage(found ? "found" : "missed");
		}
	}
	return decodedCells.size();
}

void PalletGrid::registerBarcodeMsg(std::string & msg) {
	CHECK(decodedMsgCount[msg] == 0)
	<< "decoded message found more thank once" << msg;
	++decodedMsgCount[msg];
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

std::vector<std::tr1::shared_ptr<PalletCell> > & PalletGrid::getDecodedCells() {
	return decodedCells;
}

void PalletGrid::formatCellMessages(string & msg) {
	ostringstream out;
	out << "#Plate,Row,Col,Barcode" << endl;

	for (unsigned i = 0, n = decodedCells.size(); i < n; ++i) {
		PalletCell & cell = *decodedCells[i];
		const string & msg = cell.getBarcodeMsg();
		out << plateNum << "," << static_cast<char>('A' + cell.getRow()) << ","
				<< (cell.getCol() + 1) << "," << msg << endl;
	}
	msg = out.str();
}

void PalletGrid::writeImageWithCells(std::string filename) {
	CHECK(enabledCells.size() > 0) << "cells images not initialized yet";

	RgbQuad white(255, 255, 255);
	Dib markedImage(*image);

	for (unsigned i = 0, n = enabledCells.size(); i < n; ++i) {
		enabledCells[i]->drawCellBox(markedImage, white);
	}

	markedImage.writeToFile(filename.c_str());
}

void PalletGrid::writeImageWithBoundedBarcodes(std::string filename) {
	CHECK(enabledCells.size() > 0) << "cells images not initialized yet";

	RgbQuad red(255, 0, 0);
	Dib markedImage(*image);

	std::vector<const Point *> corners;

	for (unsigned i = 0, n = enabledCells.size(); i < n; ++i) {
		if (!enabledCells[i]->getDecodeValid())
			continue;

		enabledCells[i]->drawBarcodeBox(markedImage, red);
	}

	markedImage.writeToFile(filename.c_str());
}
