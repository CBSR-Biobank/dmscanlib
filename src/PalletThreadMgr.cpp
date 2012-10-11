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

#include "DmScanLibInternal.h"
#include "PalletThreadMgr.h"
#include "PalletCell.h"
#include "Decoder.h"
#include "Dib.h"

#include <algorithm>

#ifdef WIN32
#   include <windows.h>
#   undef ERROR
#endif

#include <glog/logging.h>

using namespace std;

const unsigned PalletThreadMgr::THREAD_NUM = 8;

PalletThreadMgr::PalletThreadMgr(std::tr1::shared_ptr<Decoder> dec)
                : decoder(dec) {
}

PalletThreadMgr::~PalletThreadMgr() {
}

//first is inclusive , last is exclusive
void PalletThreadMgr::threadProcessRange(unsigned first, unsigned last) {
    for (unsigned int i = first; i < last; i++) {
        allThreads[i]->start();
    }

    for (unsigned int j = first; j < last; j++) {
        allThreads[j]->join();
    }
}

void PalletThreadMgr::threadHandler() {
    unsigned first = 0;
    unsigned last = min(numThreads, THREAD_NUM);

    do {
        threadProcessRange(first, last);
        VLOG(2)
                        << "Threads for cells finished: " << first << "/"
                        << last - 1;

        first = last;
        last = min(last + THREAD_NUM, numThreads);
    }
    while (first < numThreads);
}

void PalletThreadMgr::decodeCells(
                std::vector<std::tr1::shared_ptr<PalletCell> > & cells) {
    numThreads = cells.size();
    allThreads.resize(numThreads);

    for (unsigned i = 0; i < numThreads; ++i) {
        //cells[i]->run();
        allThreads[i] = cells[i];
    }

    threadHandler();
}

