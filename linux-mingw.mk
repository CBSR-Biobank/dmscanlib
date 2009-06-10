PROJECT := libscanlib.dll

SRC := \
	src/BinRegion.cpp \
	src/Calibrator.cpp \
	src/Decoder.cpp \
	src/Dib.c \
	src/MessageInfo.cpp \
	src/ScanLib.cpp \
	src/utils/UaLogger.cpp \
	src/utils/Util.cpp \
	libdmtx/dmtx.c \
	simpleini/ConvertUTF.c \
	src/ImageGrabber.cpp

DEBUG=1

BUILD_DIR := obj-win32
BUILD_DIR_FULL_PATH := $(CURDIR)/$(BUILD_DIR)

CC := /home/nelson/apps/cross-tools/bin/i386-mingw32-gcc
CXX := /home/nelson/apps/cross-tools/bin/i386-mingw32-g++
CFLAGS := -Wall -pedantic -fmessage-length=0 -DUA_LOGGER -DWIN32
CXXFLAGS := $(CFLAGS)
CPPFLAGS := $(CFLAGS)
SED := /bin/sed
LIBS += -lm -lstdc++
LDFLAGS := -shared -Wl,--out-implib,$(basename $(PROJECT)).a

INCLUDE_PATH := /home/nelson/apps/cross-tools/include src libdmtx src/loki src/utils simpleini 

-include common.mk
