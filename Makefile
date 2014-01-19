#***************************************************************************
#
#
#***************************************************************************

.EXPORT_ALL_VARIABLES:

PROJECT := dmscanlib

DEBUG := on

OSTYPE := $(shell uname -s | tr [:upper:] [:lower:])
MTYPE := $(shell uname -m)
LANG := en_US                # for gcc error messages
BUILD_DIR := obj
JAVA_HOME := /usr/lib/jvm/jdk1.6.0_45

SRCS := \
	src/DmScanLib.cpp \
	src/jni/DmScanLibJniLinux.cpp \
	src/jni/DmScanLibJniCommon.cpp \
	src/decoder/DecodeOptions.cpp \
	src/decoder/Decoder.cpp \
	src/decoder/DmtxDecodeHelper.cpp \
	src/decoder/WellRectangle.cpp \
	src/decoder/WellDecoder.cpp \
	src/decoder/ThreadMgr.cpp \
	src/imgscanner/ImgScanner.cpp \
	src/imgscanner/ImgScannerSimulator.cpp \
	src/utils/DmTimeLinux.cpp \
	src/test/TestWellRectangle.cpp \
	src/test/TestPoint.cpp \
	src/test/ImageInfo.cpp \
	src/test/Tests.cpp \
	src/test/TestDmScanLib.cpp \
	src/test/TestCommon.cpp \
	src/test/TestRect.cpp \
	src/test/TestBoundingBox.cpp \
	src/dib/Dib.cpp \
	src/dib/RgbQuad.cpp


FILES = $(notdir $(SRCS))
PATHS = $(sort $(dir $(SRCS) ) )
OBJS := $(addprefix $(BUILD_DIR)/, $(FILES:.cpp=.o))
DEPS := $(OBJ:.o=.d)

INCLUDE_PATH := $(foreach inc,$(PATHS),$(inc)) third_party/libdmtx third_party/glog/src \
	$(JAVA_HOME)/include $(JAVA_HOME)/include/linux
LIBS := -lglog -ldmtx -lOpenThreads -lgtest -lpthread
LIB_PATH :=

CC := g++
CXX := $(CC)
CFLAGS := -fmessage-length=0 -fPIC -std=gnu++0x
SED := /bin/sed

ifeq ($(OSTYPE),mingw32)
	HOST := windows
endif

ifeq ($(HOST),windows)
	LIBS +=
	LIB_PATH :=
endif

ifeq ($(OSTYPE),linux)
	LIBS +=
	SRC +=
	CFLAGS +=

	ifeq ($(MTYPE),x86_64)
		LIB_PATH +=
	else
		LIB_PATH +=
		LIBS +=
	endif
endif

VPATH := $(CURDIR) $(INCLUDE_PATH)

CFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))
CXXFLAGS := -pedantic -Wall -Wno-long-long -Wno-variadic-macros -Wno-deprecated \
	$(CFLAGS)
LDFLAGS += $(foreach path,$(LIB_PATH),-L$(path))
CFLAGS += -Wno-write-strings -fpermissive

ifeq ($(HOST),windows)
	CFLAGS += -D'srand48(n)=srand ((n))' -D'drand48()=((double)rand()/RAND_MAX)'
endif

ifdef DEBUG
	CFLAGS += -DDEBUG -g
	CXXFLAGS += -DDEBUG -g
#	CXXFLAGS += -D_GLIBCXX_DEBUG -DDEBUG -g
else
	CFLAGS += -O2
	CXXFLAGS += -O2
endif

ifndef VERBOSE
  SILENT := @
endif

.PHONY: all everything clean doc check-syntax

all: $(PROJECT)

$(PROJECT) : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -rf  $(BUILD_DIR)/*.[odP] $(PROJECT)

doc: doxygen.cfg
	doxygen $<

#
# This rule also creates the dependency files
#
$(BUILD_DIR)/%.o : %.cpp
	@echo "compiling $<..."
	$(SILENT)$(CXX) $(CFLAGS) -MD -o $@ $<
	$(SILENT)cp $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.P; \
	$(SED) -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(BUILD_DIR)/$*.d >> $(BUILD_DIR)/$*.P; \
	rm -f $(BUILD_DIR)/$*.d

-include $(DEPS)

# for emacs flymake
check-syntax:
	$(CXX) -Wall -Wextra -pedantic -fsyntax-only -o nul -S $(CHK_SOURCES)
