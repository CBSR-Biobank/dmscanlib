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
#include "ProcessImageManager.h"
#include "Dib.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "cxtypes.h"

#include <sstream>

#ifdef _VISUALC_
#   include <functional>
#else
#   include <tr1/functional>
#endif

PalletGrid::PalletGrid(unsigned pn, Orientation o,
                       std::tr1::shared_ptr<const Dib> img, unsigned gapX,
                       unsigned gapY, const unsigned(&profileWords)[3])
                : plateNum(pn), orientation(o), image(img) {

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

    UA_DOUT( 1,
            3,
            "PalletGrid: orientation/" << orientation << " width/" << imgWidth << " height/" << imgHeight << " imgValid/" << imgValid);

    if (!imgValid) return;

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
}

PalletGrid::~PalletGrid() {
}

void PalletGrid::applyFilters() {
    filteredImage = image->convertGrayscale();
    filteredImage->tpPresetFilter();
    UA_DEBUG(image->writeToFile("filtered.bmp"));
}

void PalletGrid::writeImageWithCells(std::string filename) {
    UA_ASSERTS(cells.size() > 0, "cells images not initialized yet");

    RgbQuad white(255, 255, 255);
    Dib markedImage(*image);

    for (unsigned i = 0, n = cells.size(); i < n; ++i) {
        std::tr1::shared_ptr<const CvRect> rect = cells[i]->getParentPos();
        markedImage.rectangle(rect->x, rect->y, rect->width, rect->height,
                                   white);
    }

    markedImage.writeToFile(filename.c_str());
}

void PalletGrid::writeImageWithBoundedBarcodes(std::string filename) {
    UA_ASSERTS(cells.size() > 0, "cells images not initialized yet");

    RgbQuad red(255, 0, 0);
    Dib markedImage(*image);

    std::vector<const CvPoint *> corners;

    for (unsigned i = 0, n = cells.size(); i < n; ++i) {
        if (!cells[i]->getDecodeValid()) continue;

        cells[i]->getCorners(corners);

        markedImage.line(*corners[0], *corners[1], red);
        markedImage.line(*corners[1], *corners[2], red);
        markedImage.line(*corners[2], *corners[3], red);
        markedImage.line(*corners[3], *corners[0], red);
    }

    markedImage.writeToFile(filename.c_str());
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

    rect.width = static_cast<int>(cellWidth) - gapX;
    rect.height = static_cast<int>(cellHeight) - gapY;
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

// TODO: crop images in decode thread
void PalletGrid::getCellImages() {
    cellsByRowCol.resize(MAX_ROWS);

    CvRect rect;
    for (unsigned row = 0, rows = MAX_ROWS; row < rows; ++row) {
        cellsByRowCol[row].resize(MAX_COLS);
        for (unsigned col = 0, cols = MAX_COLS; col < cols; ++col) {
            if (!cellEnabled[MAX_COLS * row + col]) return;

            getCellRect(row, col, rect);
            std::tr1::shared_ptr<Dib> cellImage = image->crop(
                            rect.x, rect.y, rect.x + rect.width,
                            rect.y + rect.height);
            std::tr1::shared_ptr<PalletCell> cell(
                            new PalletCell(cellImage, row, col, rect.x,
                                           rect.y));
            cells.push_back(cell);
            cellsByRowCol[row][col] = cell;

        }
    }
}

unsigned PalletGrid::decodeCells(std::tr1::shared_ptr<Decoder> decoder) {
    UA_ASSERT(imgValid);

    getCellImages();
    UA_DEBUG( string str; getProfileAsString(str); UA_DOUT(1, 5, "Profile: \n" << str); writeImageWithCells("cellRegions.bmp"););

    ProcessImageManager imageManager(decoder);
    imageManager.decodeCells(cells);

    unsigned decodedCount = 0;
    for (unsigned i = 0, n = cells.size(); i < n; ++i) {
        if (cells[i]->getDecodeValid() && !cells[i]->getBarcodeMsg().empty()) {
            ++decodedCount;
        }
    }
    return decodedCount;
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

void PalletGrid::formatCellMessages(string & msg) {
    ostringstream out;
    out << "#Plate,Row,Col,Barcode" << endl;

    for (unsigned i = 0, n = cells.size(); i < n; ++i) {
        PalletCell * cell = cells[i].get();
        const string & msg = cell->getBarcodeMsg();
        if (msg.empty()) {
            continue;
        }
        out << plateNum << "," << static_cast<char>('A' + cell->getRow()) << ","
                        << (cell->getCol() + 1) << "," << msg << endl;
    }
    msg = out.str();
}
