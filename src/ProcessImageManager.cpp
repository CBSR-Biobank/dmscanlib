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
#include "ProcessImageManager.h"
#include "BarcodeThread.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <algorithm>

const unsigned ProcessImageManager::THREAD_NUM = 8;

ProcessImageManager::ProcessImageManager(std::tr1::shared_ptr<Decoder> dec)
                : decoder(dec) {
}

ProcessImageManager::~ProcessImageManager() {
}

//first is inclusive , last is exclusive
void ProcessImageManager::threadProcessRange(unsigned first, unsigned last) {
    for (unsigned int i = first; i < last; i++) {
        allThreads[i]->start();
    }

    for (unsigned int j = first; j < last; j++) {
        allThreads[j]->join();
    }
}

void ProcessImageManager::threadHandler() {
    unsigned first = 0;
    unsigned last = std::min(numThreads, THREAD_NUM);

    do {
        threadProcessRange(first, last);
        UA_DOUT(3, 5,
                "Threads for cells finished: "<< first << "/" << last - 1);

        first = last;
        last = min(last + THREAD_NUM, numThreads);
    }
    while (first < numThreads);
}

void ProcessImageManager::decodeCells(
                std::vector<std::tr1::shared_ptr<PalletCell> > & cells) {
    numThreads = cells.size();
    allThreads.resize(numThreads);

    for (unsigned i = 0; i < numThreads; ++i) {
        std::tr1::shared_ptr<BarcodeThread> thread(
                        new BarcodeThread(decoder, cells[i]));
        allThreads[i] = thread;
    }

    threadHandler();
}
