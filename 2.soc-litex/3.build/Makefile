SHELL := /bin/bash

MAKEFILE_DIR  := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
PROJECT_ROOT  := $(realpath $(MAKEFILE_DIR)/..)

DOC_DIR       := $(PROJECT_ROOT)/0.doc
HW_DIR        := $(PROJECT_ROOT)/1.hw
SW_DIR        := $(PROJECT_ROOT)/2.sw
BUILD_DIR     := $(PROJECT_ROOT)/3.build
SIM_DIR       := $(PROJECT_ROOT)/4.sim

BAUD          ?= 115200
PORT          ?= /dev/ttyUSB0

# LiteX install
LITEX_PATH    ?= $(HOME)/FPGA/Tools/litex-hub/litex
LITEX_VENV    ?= $(LITEX_PATH)/litex-venv
PYTHON        := python3

# Board/target
BOARD         := alinx_ax7203

# ---- IMPORTANT: use your repo target script, not litex-boards target ----
TARGET_PY     := $(PROJECT_ROOT)/8.python/src/targets/alinx_ax7203_uberclock.py
PYTHONPATH_REPO := $(PROJECT_ROOT)/8.python/src

# Defaults
FREQ          ?= 100e6
OPTIONS       ?= --with-uberddr3 --with-uberclock --with-ethernet
SW_CODE_MEM   ?= main_ram
SW_DATA_MEM   ?= main_ram

SERVER_PID    := $(BUILD_DIR)/litex_server.pid
CSR_CSV_FILE  ?= $(BUILD_DIR)/build/$(BOARD)/csr.csv

.PHONY: help build-board build-sw load term \
        clean clean-gateware

help:
	@echo "Targets:"
	@echo "  make build-board [FREQ=100e6] [OPTIONS='...']"
	@echo "  make load        [FREQ=100e6] [OPTIONS='...']"
	@echo "  make term        [PORT=/dev/ttyUSB0]"
	@echo "  make build-sw    [SW_CODE_MEM=main_ram] [SW_DATA_MEM=main_ram] [WITH_CXX=1]"
	@echo "  make clean-gateware   (delete build output only)"
	@echo "  make clean            (delete build + logs + temp)"

# ----------------------------
# Build / Load using your repo target
# ----------------------------
build-board:
	@echo "[build] target=$(TARGET_PY)"
	@echo "[build] sys_clk=$(FREQ) options=$(OPTIONS)"
	source $(LITEX_VENV)/bin/activate && \
	PYTHONPATH=$(PYTHONPATH_REPO) \
	$(PYTHON) $(TARGET_PY) \
	  --build \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)
# ——————————————————————————————————————————————————————————————
#  Build & run software example under 2.sw/
# ——————————————————————————————————————————————————————————————
build-sw:
	@echo "[software build] code_mem=$(SW_CODE_MEM) data_mem=$(SW_DATA_MEM) WITH_CXX=$(WITH_CXX)"
	@echo "→ Building software in place from $(SW_DIR)"
	source $(LITEX_VENV)/bin/activate && \
	export BUILD_DIR=$(BUILD_DIR)/build/$(BOARD) && \
	export WITH_CXX=$(WITH_CXX) && \
	tmp_linker=$$(mktemp) && \
	cp $(SW_DIR)/linker.ld $$tmp_linker && \
	trap 'cp $$tmp_linker $(SW_DIR)/linker.ld; rm -f $$tmp_linker' EXIT && \
	sed -i 's/main_ram/$(SW_CODE_MEM)/g; s/data_ram/$(SW_DATA_MEM)/g' $(SW_DIR)/linker.ld && \
	$(MAKE) -C $(SW_DIR) clean && \
	$(MAKE) -C $(SW_DIR) && \
	$(PYTHON) -m litex.soc.software.crcfbigen \
	  $(SW_DIR)/demo.bin -o $(SW_DIR)/demo.fbi --fbi --little

load:
	@echo "[load] target=$(TARGET_PY)"
	source $(LITEX_VENV)/bin/activate && \
	PYTHONPATH=$(PYTHONPATH_REPO) \
	$(PYTHON) $(TARGET_PY) \
	  --load \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)

term:
	@echo "[term] port=$(PORT)"
	source $(LITEX_VENV)/bin/activate && \
	KERNEL_PATH=$$(find $(SW_DIR) -maxdepth 1 -type f -name '*.bin' | head -n1); \
	if [ -z "$${KERNEL_PATH}" ]; then \
	  echo "Error: no .bin found in $(SW_DIR)" >&2; \
	  exit 1; \
	fi; \
	litex_term $(PORT) --kernel="$${KERNEL_PATH}"

# ----------------------------
# Cleaning
# ----------------------------
clean-gateware:
	@echo "[clean] removing gateware build output"
	rm -rf $(BUILD_DIR)/build/$(BOARD)

clean:
	@echo "[clean] removing build + logs + temp"
	rm -rf $(BUILD_DIR)/build/$(BOARD)
	rm -f $(SERVER_PID)
	rm -f $(BUILD_DIR)/litex_server.log
