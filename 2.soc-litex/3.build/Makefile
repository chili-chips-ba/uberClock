# 3.build/Makefile
SHELL := /bin/bash

# ------------------------------------------------------------------
#  Compute project‐root and subdirectories relative to this Makefile
# ------------------------------------------------------------------
MAKEFILE_DIR  := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
PROJECT_ROOT  := $(realpath $(MAKEFILE_DIR)/..)
DOC_DIR       := $(PROJECT_ROOT)/0.doc
HW_DIR        := $(PROJECT_ROOT)/1.hw
SW_DIR        := $(PROJECT_ROOT)/2.sw
BUILD_DIR     := $(PROJECT_ROOT)/3.build
SIM_DIR       := $(PROJECT_ROOT)/4.sim

# ------------------------------------------------------------------
#  Litex / Python virtual‐env settings (override via env)
# ------------------------------------------------------------------
# where your litex checkout lives
LITEX_ROOT      ?= $(HOME)/FPGA/Tools/litex-hub/litex
# where your venv activate script lives
VENV_ACTIVATE   ?= $(LITEX_ROOT)/litex-venv/bin/activate
PYTHON          := python3

# ------------------------------------------------------------------
#  SoC & example definitions
# ------------------------------------------------------------------
BOARD        := alinx_ax7203
BOARDS_PKG   := litex_boards.targets.$(BOARD)
EXAMPLE_DIR  := $(SW_DIR)
EDITOR       := code

# ------------------------------------------------------------------
#  Defaults (override on CLI)
# ------------------------------------------------------------------
FREQ         ?= 50e6
OPTIONS      ?=
PORT         ?= /dev/ttyUSB1
DEMO_FLAGS   ?= --with-cxx

# simulation settings
SIM_FREQ      ?= $(FREQ)
SIM_RAM_BYTES ?= 0x20000
SIM_JSON      := $(SIM_DIR)/soc.json
SIM_CSV       := $(SIM_DIR)/csr.csv
SIM_BIOS      ?= $(EXAMPLE_DIR)/cordic/demo.bin

.PHONY: help open-target open-example build-board load term \
        build-sw sim view-sim clean-sim

help:
	@echo "Usage:"
	@echo "  make open-target   file=<name>"
	@echo "  make open-example  file=<proj>"
	@echo "  make build-board   [FREQ=..] [OPTIONS='..']"
	@echo "  make load          [FREQ=..] [OPTIONS='..']"
	@echo "  make term               [PORT=..]"
	@echo "  make build-sw           [DEMO_FLAGS='..']"
	@echo "  make sim"
	@echo "  make view-sim"
	@echo "  make clean-sim"

# ——————————————————————————————————————————————————————————————
#  Open the target or example in your editor
# ——————————————————————————————————————————————————————————————
open-target:
	$(EDITOR) $(LITEX_ROOT)/litex-boards/litex_boards/targets/$(file).py

open-example:
	$(EDITOR) $(EXAMPLE_DIR)/$(file)/

# ——————————————————————————————————————————————————————————————
#  Board build & load
# ——————————————————————————————————————————————————————————————
build-board:
	@echo "[litex build] freq=$(FREQ) opts='$(OPTIONS)'"
	source $(VENV_ACTIVATE) && \
	$(PYTHON) -m $(BOARDS_PKG) \
	  --build \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)

load:
	@echo "[litex load]"
	source $(VENV_ACTIVATE) && \
	$(PYTHON) -m $(BOARDS_PKG) \
	  --load \
	  $(OPTIONS)

# ——————————————————————————————————————————————————————————————
#  Serial terminal into built example
# ——————————————————————————————————————————————————————————————
term:
	@echo "[serial term] port=$(PORT)"
	source $(VENV_ACTIVATE) && \
	KERNEL_PATH=$$(find $(SW_DIR) -maxdepth 1 -type f -name '*.bin' | head -n1); \
	if [ -z "$${KERNEL_PATH}" ]; then \
	  echo "Error: no .bin found in $(SW_DIR)" >&2; \
	  exit 1; \
	fi; \
	litex_term $(PORT) --kernel="$${KERNEL_PATH}"

# ——————————————————————————————————————————————————————————————
#  Build & run software example under 2.sw/
# ——————————————————————————————————————————————————————————————
build-sw:
	@echo "[software build] DEMO_FLAGS='$(DEMO_FLAGS)'"
	@echo "→ Executing demo.py in $(SW_DIR)"
	source $(VENV_ACTIVATE) && \
	cd $(SW_DIR) && \
	./demo.py \
	  --build-path=$(BUILD_DIR)/build/$(BOARD) \
	  --mem=main_ram \
	  $(DEMO_FLAGS)

# ——————————————————————————————————————————————————————————————
#  Verilator-based simulation (IN PROGRESS)
# ——————————————————————————————————————————————————————————————
$(SIM_DIR):
	mkdir -p $(SIM_DIR)

$(SIM_JSON) $(SIM_CSV): | $(SIM_DIR)
	@echo "[sim-gen] soc.json & csr.csv @ $(SIM_FREQ)Hz"
	source $(VENV_ACTIVATE) && \
	$(PYTHON) -m $(BOARDS_PKG) \
	  --sys-clk-freq=$(SIM_FREQ) \
	  --with-cordic \
	  --integrated-main-ram-size=$(SIM_RAM_BYTES) \
	  --soc-json=$(SIM_JSON) \
	  --csr-csv=$(SIM_CSV) \
	  --no-compile

.PHONY: sim
sim: $(SIM_JSON) $(SIM_CSV)
	@echo "[sim] Verilator + litex_sim @ $(SIM_FREQ)Hz, BIOS=$(SIM_BIOS)"
	source $(VENV_ACTIVATE) && \
	litex_sim \
	  --soc-json=$(SIM_JSON) \
	  --csr-json=$(SIM_CSV) \
	  --rom-init=$(SIM_BIOS) \
	  --trace \
	  --output-dir=$(SIM_DIR) \
	  --no-compile-software \
	  --no-timer

view-sim:
	@echo "[view-sim] opening VCD in $(SIM_DIR)"
	gtkwave $$(ls $(SIM_DIR)/*.vcd | head -n1)

clean-sim:
	@echo "[clean-sim] removing sim artifacts"
	rm -rf $(SIM_DIR)/*.{json,csv,vcd,fst}
