#ifndef BARCODE_THREAD_H_
#define BARCODE_THREAD_H_
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

#include "cv.h"
#include "dmtx.h"

#include <vector>
#include <memory>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

#if !defined _VISUALC_
#   include <tr1/memory>
#endif

using namespace std;

class Decoder;
class PalletCell;
class BarcodeInfo;

class BarcodeThread: public OpenThreads::Thread {
public:
    BarcodeThread(std::tr1::shared_ptr<Decoder> decoder, std::tr1::shared_ptr<PalletCell> cell);

    virtual ~ BarcodeThread();

    virtual void run();

    bool isFinished();

private:
    std::tr1::shared_ptr<Decoder> decoder;
    std::tr1::shared_ptr<PalletCell> cell;
    DmtxImage * image;
    OpenThreads::Mutex quitMutex;
    volatile bool quitFlag;
    unsigned dpi;
    bool debug;
};

#endif /* BARCODE_THREAD_H_ */

