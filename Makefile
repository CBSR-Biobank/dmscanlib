PROJECT := scanlib

ifeq ($(OSTYPE),msys)
  PROJECT := $(PROJECT).exe
endif

SRC := \
	src/BinRegion.cpp \
	src/Calibrator.cpp \
	src/Decoder.cpp \
	src/Dib.c \
	src/MessageInfo.cpp \
	src/ScanLib.cpp \
	src/utils/UaDebug.cpp \
	src/utils/Util.cpp \
	libdmtx/dmtx.c \
	simpleini/ConvertUTF.c

ifeq ($(OSTYPE),msys)
SRC += \
	src/ImageGrabber.cpp
endif

DEBUG=1

BUILD_DIR := obj
BUILD_DIR_FULL_PATH := $(CURDIR)/$(BUILD_DIR)

CC := gcc
CXX := g++
CFLAGS := -Wall -pedantic -fmessage-length=0 -DUA_HAVE_DEBUG
CXXFLAGS := $(CFLAGS)
CPPFLAGS := $(CFLAGS)
SED := /bin/sed
LIBS += -lm -lstdc++

INCLUDE_PATH := src libdmtx src/loki src/utils simpleini

-include common.mk
