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

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

class Decoder;
class Dib;
class PalletCell;
struct CvRect;

class PalletGrid {
public:
    enum Orientation {
        ORIENTATION_HORIZONTAL, ORIENTATION_VERTICAL
    };

    static const unsigned MAX_ROWS = 8;
    static const unsigned MAX_COLS = 12;
    static const unsigned NUM_CELLS = MAX_ROWS * MAX_COLS;

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
    PalletGrid(unsigned plateNum, Orientation o,
               std::tr1::shared_ptr<const Dib> image, unsigned gapX,
               unsigned gapY, const unsigned(&profileWords)[3]);

    ~PalletGrid();

    void applyFilters();

    std::tr1::shared_ptr<const Dib> getCellImage(unsigned row, unsigned col);

    /**
     * Returns wether or not the pallet location is enabled via the profile.
     *
     * @param row a number between 0 and PalletGrid::MAX_ROWS - 1.
     * @param col a number between 0 and PalletGrid::MAX_COLS - 1.
     *
     * @return true if the cell should be decoded.
     */
    bool getCellEnabled(unsigned row, unsigned col);

    void getPositionStr(unsigned row, unsigned col, std::string & str);

    /**
     * Returns the number of cells that were decoded.
     */
    unsigned decodeCells(std::tr1::shared_ptr<Decoder> dcdr);

    void getProfileAsString(std::string & str);

    /**
     * Writes a bitmap to disc with the cell bounded with a white box.
     */
    void writeImageWithCells(std::string filename);

    /**
     * Writes a bitmap to disc with the decoded barcodes surrounded with a red box.
     */
    void writeImageWithBoundedBarcodes(std::string filename);

    bool isImageValid() {
        return imgValid;
    }

    void formatCellMessages(std::string & msg);

private:
    void getCellImages();
    void getCellRect(unsigned row, unsigned col, CvRect & rect);

    unsigned plateNum;
    Orientation orientation;

    std::vector<std::tr1::shared_ptr<PalletCell> > cells;

    std::vector<std::vector<std::tr1::shared_ptr<PalletCell> > > cellsByRowCol;

    std::tr1::shared_ptr<const Dib> image;
    std::tr1::shared_ptr<Dib> filteredImage;
    std::tr1::shared_ptr<Decoder> decoder;
    double cellWidth;
    double cellHeight;
    unsigned gapX;
    unsigned gapY;
    std::vector<bool> cellEnabled;
    bool imgValid;

};

#endif /* PALLET_GRID_H_ */
