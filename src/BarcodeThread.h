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

using namespace std;

class Dib;
class BarcodeInfo;
class ProcessImageManager;

class BarcodeThread: public OpenThreads::Thread {
public:
    BarcodeThread(ProcessImageManager * manager, double scanGap,
            unsigned squareDev, unsigned edgeThresh, unsigned corrections,
            CvRect & croppedOffset, auto_ptr<Dib> dib, BarcodeInfo & info);

    virtual ~ BarcodeThread();

    virtual void run();

    bool isFinished();

private:

    auto_ptr<Dib> dib;
    DmtxImage * image;
    CvRect croppedOffset;
    ProcessImageManager * manager;
    BarcodeInfo & barcodeInfo;

    OpenThreads::Mutex quitMutex;
    volatile bool quitFlag;

    void writeDib(const char * basename);
    void writeDiagnosticImage(DmtxDecode *dec);

    double scanGap;
    unsigned squareDev;
    unsigned edgeThresh;
    unsigned corrections;
    unsigned dpi;

    bool debug;
};

#endif /* BARCODE_THREAD_H_ */

