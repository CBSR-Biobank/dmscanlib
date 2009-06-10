PROJECT := scanlib.exe

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

BUILD_DIR := obj
BUILD_DIR_FULL_PATH := $(CURDIR)/$(BUILD_DIR)

CC := gcc
CXX := g++
CFLAGS := -Wall -pedantic -fmessage-length=0 -DUA_LOGGER
CXXFLAGS := $(CFLAGS)
CPPFLAGS := $(CFLAGS)
SED := sed
LIBS += -lm -lstdc++
LDFLAGS := -shared -Wl,--out-implib,$(basename $(PROJECT)).a

INCLUDE_PATH := src libdmtx src/loki src/utils simpleini

include common.mk
