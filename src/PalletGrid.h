#ifndef PALLET_GRID_H_
#define PALLET_GRID_H_

/*
 * CellPosition.h
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
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

#include <string>
#include <vector>

struct CvRect;

using namespace std;

class PalletGrid {
public:
	enum Orientation {
		ORIENTATION_HORIZONTAL, ORIENTATION_VERTICAL
	};

	static const unsigned MAX_ROWS;
	static const unsigned MAX_COLS;
	static const unsigned NUM_CELLS;

	/**
	 * Constructs a pallet grid with cells cellWidth pixels and
	 * cellHeight pixels.
	 *
	 * @param o The orientation of the grid. Either ORIENTATION_HORIZONTAL
	 * or ORIENTATION_VERTICAL.
	 *
	 * @param imgWidth The width of the image in pixels.
	 *
	 * @param imgHeight The height of the image in pixels.
	 *
	 * @param gapX The horizontal gap between cells in pixels.
	 *
	 * @param gapY The vertical gap between cells in pixels.
	 */
	PalletGrid(Orientation o, unsigned imgWidth, unsigned imgHeight,
			unsigned gapX, unsigned gapY, const unsigned(&profileWords)[3]);

	~PalletGrid();

	/**
	 * Returns wether or not the pallet location should be decoded.
	 *
	 * @param row a number between 0 and PalletGrid::MAX_ROWS - 1.
	 * @param col a number between 0 and PalletGrid::MAX_COLS - 1.
	 *
	 * @return true if the cell should be decoded.
	 */
	bool getCellEnabled(unsigned row, unsigned col);

	void getCellRect(unsigned row, unsigned col, CvRect & rect);

	void getPositionStr(unsigned row, unsigned col, string & str);

	bool isImageValid() {
		return imgValid;
	}

private:

	Orientation orientation;
	double cellWidth;
	double cellHeight;
	unsigned gapX;
	unsigned gapY;
	vector<bool> bits;
	bool imgValid;

};

#endif /* PALLET_GRID_H_ */
