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

SRC := \
	DecodedWell.cpp \
	WellRectangle.cpp \
	WellDecoder.cpp \
	Dib.cpp \
	RgbQuad.cpp \
	PalletThreadMgr.cpp \
	DmScanLib.cpp \
	Decoder.cpp \
	TestApp.cpp \
	ImgScanner.cpp \
	edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_common.cpp \
	edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_linux.cpp \
	TimeUtilLinux.cpp \
	ImgScannerSimulator.cpp


INCLUDE_PATH := . src src/utils third_party/libdmtx third_party/glog/src \
	/usr/lib/jvm/jdk1.6.0_32/include /usr/lib/jvm/jdk1.6.0_32/include/linux
LIBS := -lglog -ldmtx -lOpenThreads
LIB_PATH :=

BUILD_DIR := obj
BUILD_DIR_FULL_PATH := $(CURDIR)/$(BUILD_DIR)

CC := g++
CXX := $(CC)
CFLAGS := -fmessage-length=0 -fPIC
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

VPATH := $(CURDIR) $(INCLUDE_PATH) $(BUILD_DIR)

OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
DEPS := $(addsuffix .d, $(basename $(notdir $(SRC))))

OBJS_RELATIVE_PATH := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEPS_FULL_PATH := $(addprefix $(CURDIR)/$(BUILD_DIR)/, $(DEPS))

CFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))
CXXFLAGS := -pedantic -Wall -Wno-long-long -Wno-variadic-macros -Wno-deprecated \
	$(CFLAGS)
CPPFLAGS := $(CFLAGS)
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

clean:
	rm -rf  $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(PROJECT)

doc: doxygen.cfg
	doxygen $<

$(PROJECT) : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS_RELATIVE_PATH) $(LIBS)

client:
	$(MAKE) -C Competition/Client

%.o : %.cpp
	@echo "compiling $<..."
	$(SILENT) $(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $<

%.o : %.c
	@echo "compiling $<..."
	$(SILENT) $(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $<

#------------------------------------------------------------------------------
# Include the dependency files.  If they don't exist, then silent ignore it.
#
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS_FULL_PATH)
endif

#------------------------------------------------------------------------------
# Dependency rules
#
# need to use full paths here because make 3.80 cant be told what directories
# to look for for for included makefiles.
#
$(CURDIR)/$(BUILD_DIR)/%.d : %.c
	@echo "updating dependencies for $(notdir $@)..."
	$(SILENT) test -d "$(BUILD_DIR_FULL_PATH)" || mkdir -p "$(BUILD_DIR_FULL_PATH)"
	$(SILENT) $(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< \
		| $(SED) '\''s|\($(notdir $*)\)\.o[ :]*|\1.o $@: |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

$(CURDIR)/$(BUILD_DIR)/%.d : %.cpp
	@echo "updating dependencies for $(notdir $@)..."
	$(SILENT) test -d "$(BUILD_DIR_FULL_PATH)" || mkdir -p "$(BUILD_DIR_FULL_PATH)"
	$(SILENT) $(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< \
		| $(SED) '\''s|\($(notdir $*)\)\.o[ :]*|\1.o $@: |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

# for emacs flymake
check-syntax:
	$(CXX) -Wall -Wextra -pedantic -fsyntax-only -o nul -S $(CHK_SOURCES)
