#
# meant to be included by a top level makefile
#

VPATH := $(CURDIR) $(INCLUDE_PATH) $(BUILD_DIR)

OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
DEPS := $(addsuffix .d, $(basename $(notdir $(SRC))))

OBJS_RELATIVE_PATH := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEPS_FULL_PATH := $(addprefix $(CURDIR)/$(BUILD_DIR)/, $(DEPS))

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

.PHONY: all everything clean doc

all: $(PROJECT)

all2:
	@echo "DEPS: $(DEPS_FULL_PATH)"

clean:
	@echo "cleaning $(PROJECT)"
	$(SILENT) rm -rf  $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(PROJECT)

$(PROJECT) : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS_RELATIVE_PATH) $(LIBS)

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
	$(SILENT) $(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
		| $(SED) '\''s|\($(notdir $*)\)\.o[ :]*|\1.o $@: |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

