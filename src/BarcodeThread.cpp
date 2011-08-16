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

#include "BarcodeThread.h"
#include "DecodeInfo.h"
#include "PalletCell.h"
#include "Dib.h"
#include "Decoder.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "ProcessImageManager.h"
#include "cxtypes.h"

#ifdef _VISUALC_
#   include <functional>
#else
#   include <tr1/functional>
#endif

BarcodeThread::BarcodeThread(std::tr1::shared_ptr<Decoder> dec,
		std::tr1::shared_ptr<PalletCell> c) :
		decoder(dec), cell(c) {

	dpi = cell->getImage()->getDpi();

	quitMutex.lock();
	this->quitFlag = false;
	quitMutex.unlock();
}

BarcodeThread::~BarcodeThread() {
	dmtxImageDestroy(&image);
}

void BarcodeThread::run() {
    Decoder::DecodeCallback callback = std::tr1::bind(
            &BarcodeThread::decodeCallback, this, std::tr1::placeholders::_1,
            std::tr1::placeholders::_2, std::tr1::placeholders::_3);

	decoder->decodeImage(*cell->getImage().get(), callback);

	quitMutex.lock();
	quitFlag = true;
	quitMutex.unlock();
	return;
}

void BarcodeThread::decodeCallback(DmtxDecode *dec, DmtxRegion * reg,
                                   DmtxMessage * msg) {

}

bool BarcodeThread::isFinished() {
	bool quitFlagBuf;

	quitMutex.lock();
	quitFlagBuf = quitFlag;
	quitMutex.unlock();

	return quitFlagBuf;
}

