PYTHON ?= python3
BUILD_DIR ?= ../build/
APP_BUILD_DIR ?= $(CURDIR)/build
CODE_MEM ?= main_ram
DATA_MEM ?= main_ram

include $(BUILD_DIR)/software/include/generated/variables.mak

LITEX_DIRECTORY ?= $(shell $(PYTHON) -c 'import os, litex; print(os.path.dirname(litex.__file__))')
COMMON_MAK := $(if $(SOC_DIRECTORY),$(SOC_DIRECTORY)/software/common.mak,$(LITEX_DIRECTORY)/soc/software/common.mak)
include $(COMMON_MAK)

.DEFAULT_GOAL := all

# ------------------------------------------------------------------------------
# Paths
# ------------------------------------------------------------------------------
UBERCLOCK_DIR := $(CURDIR)/uberclock
SRC_DIR := $(UBERCLOCK_DIR)/src
INC_DIR := $(UBERCLOCK_DIR)/inc
KISSFFT_DIR := $(UBERCLOCK_DIR)/kissfft

OBJ_DIR := $(APP_BUILD_DIR)/obj
ELF := $(APP_BUILD_DIR)/demo.elf
BIN := $(APP_BUILD_DIR)/demo.bin
LINKER_SCRIPT := $(APP_BUILD_DIR)/linker.ld

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------
TOP_C_SOURCES := \
	$(CURDIR)/console.c \
	$(CURDIR)/donut.c \
	$(CURDIR)/helloc.c \
	$(CURDIR)/ubddr3.c

UBERCLOCK_C_SOURCES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/uberclock_channels.c \
	$(SRC_DIR)/uberclock_commands.c \
	$(SRC_DIR)/uberclock_dma.c \
	$(SRC_DIR)/uberclock_fft.c \
	$(SRC_DIR)/uberclock_fifo.c \
	$(SRC_DIR)/uberclock_hw.c \
	$(SRC_DIR)/uberclock_parse.c \
	$(SRC_DIR)/uberclock_siggen.c \
	$(SRC_DIR)/uberclock_track.c

KISSFFT_C_SOURCES := \
	$(KISSFFT_DIR)/kiss_fft.c

APP_CPP_SOURCES :=

ifdef WITH_CXX
APP_CPP_SOURCES += $(CURDIR)/hellocpp.cpp
CPPFLAGS += -DWITH_CXX
endif

# ------------------------------------------------------------------------------
# Objects
# ------------------------------------------------------------------------------
OBJECTS := \
	$(patsubst $(CURDIR)/%.c,$(OBJ_DIR)/%.o,$(TOP_C_SOURCES)) \
	$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/uberclock/src/%.o,$(UBERCLOCK_C_SOURCES)) \
	$(patsubst $(KISSFFT_DIR)/%.c,$(OBJ_DIR)/uberclock/kissfft/%.o,$(KISSFFT_C_SOURCES)) \
	$(patsubst $(CURDIR)/%.cpp,$(OBJ_DIR)/%.o,$(APP_CPP_SOURCES))

DEPFILES := $(OBJECTS:.o=.d)

# ------------------------------------------------------------------------------
# Flags
# ------------------------------------------------------------------------------
CPPFLAGS += -I$(CURDIR)
CPPFLAGS += -I$(INC_DIR)
CPPFLAGS += -I$(KISSFFT_DIR)
CPPFLAGS += -DFIXED_POINT=16

$(OBJ_DIR)/donut.o: CFLAGS += -w

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------
all: $(BIN)

demo.bin: $(BIN)
	cp $< $@

$(BIN): $(ELF) | $(APP_BUILD_DIR)
	$(OBJCOPY) -O binary $< $@
ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

$(ELF): $(OBJECTS) $(LINKER_SCRIPT) | $(APP_BUILD_DIR)
	$(CC) $(LDFLAGS) -T $(LINKER_SCRIPT) -N -o $@ \
		$(OBJECTS) \
		$(PACKAGES:%=-L$(BUILD_DIR)/software/%) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-Wl,-Map,$@.map \
		$(LIBS:lib%=-l%)
ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

$(LINKER_SCRIPT): $(CURDIR)/linker.ld | $(APP_BUILD_DIR)
	sed \
		-e 's/main_ram/$(CODE_MEM)/g' \
		-e 's/data_ram/$(DATA_MEM)/g' \
		$< > $@

$(APP_BUILD_DIR):
	mkdir -p $@

# ------------------------------------------------------------------------------
# Compile rules
# ------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: $(CURDIR)/%.c | $(APP_BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo " CC       $@"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/uberclock/src/%.o: $(SRC_DIR)/%.c | $(APP_BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo " CC       $@"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/uberclock/kissfft/%.o: $(KISSFFT_DIR)/%.c | $(APP_BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo " CC       $@"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(CURDIR)/%.cpp | $(APP_BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo " CXX      $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPFILES)

clean:
	rm -rf $(APP_BUILD_DIR) demo.bin demo.elf demo.fbi

.PHONY: all clean
