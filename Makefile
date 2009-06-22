PROJECT := scanlib.exe

SRC := \
	src/BinRegion.cpp \
	src/Calibrator.cpp \
	src/Config.cpp \
	src/Decoder.cpp \
	src/Dib.c \
	src/BarcodeInfo.cpp \
	src/testapp.cpp \
	src/utils/UaLogger.cpp \
	src/utils/Util.cpp \
	libdmtx/dmtx.c \
	simpleini/ConvertUTF.c

DEBUG=1

BUILD_DIR := obj
BUILD_DIR_FULL_PATH := $(CURDIR)/$(BUILD_DIR)

CC := gcc
CXX := g++
CFLAGS := -Wall -pedantic -fmessage-length=0 -DUA_LOGGER -D_DEBUG
CXXFLAGS := $(CFLAGS)
CPPFLAGS := $(CFLAGS)
SED := sed
LIBS += -lm -lstdc++
LDFLAGS :=

INCLUDE_PATH := src libdmtx src/loki src/utils simpleini simpleopt

include common.mk
