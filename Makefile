PROJECT := scanlib

s+ = $(subst \\ ,+,$1)
+s = $(subst +,\\ ,$1)

CURDIR_NO_SPACES := $(call s+,$(CURDIR))

ifeq ($(OSTYPE),msys)
  PROJECT := $(PROJECT).exe
endif

SRC := \
	src/Decoder.cpp \
	src/Dib.c \
	src/ImageProcessor.cpp \
	src/ScanLib.cpp \
	src/utils/UaDebug.cpp \
	src/utils/LinkList.cpp \
	libdmtx/dmtx.c 

ifeq ($(OSTYPE),msys)
SRC += \
	src/ImageGrabber.cpp
endif	

DEBUG=1

BUILD_DIR := obj
BUILD_DIR_FULL_PATH := $(CURDIR_NO_SPACES)/$(BUILD_DIR)

CC := gcc
CXX := g++
CFLAGS := -Wall -pedantic -fmessage-length=0 -DUA_HAVE_DEBUG
CXXFLAGS := $(CFLAGS)
CPPFLAGS := $(CFLAGS)
SED := /bin/sed
LIBS += -lm -lstdc++

INCLUDE_PATH := src libdmtx src/loki src/utils iniParser
VPATH := $(CURDIR_NO_SPACES) $(INCLUDE_PATH) $(BUILD_DIR)

OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
DEPS := $(addsuffix .d, $(basename $(notdir $(SRC))))

OBJS_RELATIVE_PATH := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEPS_FULL_PATH := $(addprefix  $(CURDIR_NO_SPACES)/$(BUILD_DIR)/, $(DEPS))

CFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))
CXXFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))
CPPFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))

ifdef DEBUG
	CFLAGS += -DDEBUG -g
	CXXFLAGS += -DDEBUG -g
else
	CFLAGS += -O2
	CXXFLAGS += -O2
    LIBS += -lc
endif

ifndef VERBOSE
  SILENT := @
endif

ifeq ($(OSTYPE),msys)
	CFLAGS += -DWIN32
	CXXFLAGS += -DWIN32
else	
	CFLAGS += -D_UNIX_ -fPIC
	CXXFLAGS += -D_UNIX_ -fPIC
endif

.PHONY: all everything clean doc

all:
	@echo "Deps: $(DEPS_FULL_PATH)"

all2: $(PROJECT)

clean:
	@echo "cleaning $(PROJECT)"
	$(SILENT) rm -rf  $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(PROJECT)

$(PROJECT) : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS_RELATIVE_PATH) $(LIBS)

%.o : %.cpp
	@echo "compiling $<..."
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$(BUILD_DIR)/$@" "$<"

%.o : %.c
	@echo "compiling $<..."
	$(SILENT) $(CC) $(CFLAGS) -o "$(BUILD_DIR)/$@" "$<"



#------------------------------------------------------------------------------
# Include the dependency files.  If they don't exist, then silent ignore it.
#
ifneq ($(MAKECMDGOALS),clean)
#-include $(DEPS_FULL_PATH)
#-include $(DEPS)
endif

#------------------------------------------------------------------------------
# Dependency rules
#
# need to use full paths here because make 3.80 cant be told what directories
# to look for for for included makefiles.
#
$(CURDIR_NO_SPACES)/$(BUILD_DIR)/%.d : %.c
	@echo "updating dependencies for $(notdir "$@")..."
	$(SILENT) test -d "$(BUILD_DIR_FULL_PATH)" || mkdir -p "$(BUILD_DIR_FULL_PATH)"
	$(SILENT) $(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< \
		| $(SED) '\''s|\($(notdir $*)\)\.o[ :]*|\1.o $@: |g'\'' > "$@"; \
		[ -s "$@" ] || rm -f "$@"'

$(CURDIR_NO_SPACES)/$(BUILD_DIR)/%.d" : %.cpp
	@echo "updating dependencies for $(notdir "$@")..."
	$(SILENT) test -d "$(BUILD_DIR_FULL_PATH)" || mkdir -p "$(BUILD_DIR_FULL_PATH)"
	$(SILENT) $(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
		| $(SED) '\''s|\($(notdir $*)\)\.o[ :]*|\1.o $@: |g'\'' > "$@"; \
		[ -s "$@" ] || rm -f "$@"'

