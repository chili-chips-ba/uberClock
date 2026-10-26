# SPDX-FileCopyrightText: 2026 Ahmed Imamović
# SPDX-FileCopyrightText: 2026 Tarik Hamedović
# SPDX-License-Identifier: GPL-3.0-or-later

SHELL := /bin/bash

MAKEFILE_DIR  := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPO_ROOT     := $(realpath $(MAKEFILE_DIR)/..)
SOC_ROOT      := $(REPO_ROOT)/2.soc

DOC_DIR       := $(REPO_ROOT)/0.doc
DOXYFILE      := $(DOC_DIR)/Doxyfile
DOCS_DIR      := $(DOC_DIR)/docs
DSP_DIR       := $(REPO_ROOT)/1.dsp
HW_DIR        := $(SOC_ROOT)/1.hw
SW_DIR        := $(SOC_ROOT)/2.sw
BUILD_DIR     := $(REPO_ROOT)/3.build
SCRIPTS_DIR   := $(BUILD_DIR)/scripts
SIM_DIR       := $(SOC_ROOT)/4.sim
PYTHON_PKG_DIR := $(SOC_ROOT)/8.python

BAUD          ?= 115200
PORT          ?= /dev/ttyUSB0

# Python / environment selection
DEFAULT_VENV  := $(firstword $(wildcard $(REPO_ROOT)/.venv $(HOME)/litex-venv $(HOME)/.venv/litex))
PYTHON        ?= $(if $(VIRTUAL_ENV),$(VIRTUAL_ENV)/bin/python3,$(if $(DEFAULT_VENV),$(DEFAULT_VENV)/bin/python3,python3))
PYTHON_BIN_DIR := $(dir $(PYTHON))
PYTHONPATH_REPO ?= $(PYTHON_PKG_DIR)/src
SOC_MODULE    ?= uberclock_soc

# Board/target
BOARD         := alinx_ax7203

# Defaults
FREQ          ?= 100e6
OPTIONS       ?= --with-uberddr3 --with-uberclock --with-ethernet
SW_CODE_MEM   ?= main_ram
SW_DATA_MEM   ?= main_ram
SIM_CFLAGS    ?= -Wno-error=incompatible-pointer-types
SIM_CYCLES    ?= 100000

# Toolchain: vivado (default) or openxc7 (Yosys + nextpnr-xilinx + prjxray,
# no Vivado; see docs/openxc7_port/main.tex for the issues this works around)
TOOLCHAIN     ?= vivado

# openXC7 1.0.0 install (toolchain-sources-builder.sh layout; default prefix
# /opt/openxc7, override OPENXC7_DIR for a custom INSTALL_PREFIX). CHIPDB must
# hold a pre-generated himbaechel chipdb named xc7a200tfbg484.bin.
OPENXC7_DIR               ?= /opt/openxc7
CHIPDB                    ?= $(HOME)/openxc7-chipdb
PRJXRAY_DB_DIR            ?= $(OPENXC7_DIR)/share/nextpnr/prjxray-db
NEXTPNR_XILINX_PYTHON_DIR ?= $(OPENXC7_DIR)/lib/python

SERVER_PID    := $(BUILD_DIR)/litex_server.pid
CSR_CSV_FILE  ?= $(BUILD_DIR)/build/$(BOARD)/csr.csv

.PHONY: help build-board build-sw sim-soc sim-soc-run load term \
        clean clean-gateware clean-sw clean-sw-deep \
        install-python print-python docs view-docs clean-docs

help:
	@echo "Targets:"
	@echo "  make build-board [TOOLCHAIN=vivado|openxc7] [FREQ=100e6] [OPTIONS='...']"
	@echo "  make sim-soc     [SIM_OPTIONS='...']"
	@echo "  make sim-soc-run [SIM_OPTIONS='...']"
	@echo "  make load        [FREQ=100e6] [OPTIONS='...']"
	@echo "  make term        [PORT=/dev/ttyUSB0]"
	@echo "  make build-sw    [SW_CODE_MEM=main_ram] [SW_DATA_MEM=main_ram] [WITH_CXX=1]"
	@echo "  make clean-sw        (remove software build outputs only)"
	@echo "  make clean-sw-deep   (remove software outputs + maps)"
	@echo "  make docs            (build Sphinx docs)"
	@echo "  make view-docs       (open built docs in browser)"
	@echo "  make clean-docs      (remove built docs)"
	@echo "  make install-python  (editable install of 8.python into $(PYTHON))"
	@echo "  make print-python    (show which interpreter will be used)"
	@echo "  make clean-gateware  (delete gateware build output only)"
	@echo "  make clean           (delete build + logs + temp)"
	@echo ""
	@echo "Helper scripts live under $(SCRIPTS_DIR)"

print-python:
	@echo "PYTHON=$(PYTHON)"
	@echo "PIP=$(PYTHON) -m pip"
	@echo "PYTHONPATH_REPO=$(PYTHONPATH_REPO)"

install-python:
	$(PYTHON) -m pip install -e $(PYTHON_PKG_DIR)

# ----------------------------
# Build / Load using your repo target
# ----------------------------
build-board:
	@echo "[build] module=$(SOC_MODULE)"
	@echo "[build] python=$(PYTHON)"
	@echo "[build] toolchain=$(TOOLCHAIN) sys_clk=$(FREQ) options=$(OPTIONS)"
	if [ "$(TOOLCHAIN)" = "openxc7" ]; then \
	  mkdir -p "$(CHIPDB)"; \
	  export PATH="$(OPENXC7_DIR)/bin:$(PYTHON_BIN_DIR):$$PATH"; \
	  export CHIPDB="$(CHIPDB)"; \
	  export PRJXRAY_DB_DIR="$(PRJXRAY_DB_DIR)"; \
	  export NEXTPNR_XILINX_PYTHON_DIR="$(NEXTPNR_XILINX_PYTHON_DIR)"; \
	fi; \
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) -m $(SOC_MODULE) \
	  --build \
	  --toolchain=$(TOOLCHAIN) \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)

sim-soc:
	@echo "[sim] module=$(SOC_MODULE).sim"
	@echo "[sim] python=$(PYTHON)"
	CFLAGS="$(SIM_CFLAGS)" \
	PATH=$(PYTHON_BIN_DIR):$$PATH \
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) -m $(SOC_MODULE).sim \
	  --with-uberclock \
	  --build \
	  --no-run \
	  --output-dir=$(BUILD_DIR)/build/sim \
	  $(SIM_OPTIONS)

sim-soc-run:
	@echo "[sim] module=$(SOC_MODULE).sim"
	@echo "[sim] python=$(PYTHON)"
	CFLAGS="$(SIM_CFLAGS)" \
	PATH=$(PYTHON_BIN_DIR):$$PATH \
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) -m $(SOC_MODULE).sim \
	  --with-uberclock \
	  --build \
	  --non-interactive \
	  --trace \
	  --finish-after-cycles=$(SIM_CYCLES) \
	  --output-dir=$(BUILD_DIR)/build/sim \
	  $(SIM_OPTIONS)
# ----------------------------
# Build software under 2.sw/
# ----------------------------
build-sw:
	@echo "[software build] code_mem=$(SW_CODE_MEM) data_mem=$(SW_DATA_MEM) WITH_CXX=$(WITH_CXX)"
	@echo "→ Building software in place from $(SW_DIR)"
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
	@echo "[load] module=$(SOC_MODULE)"
	@echo "[load] python=$(PYTHON)"
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) -m $(SOC_MODULE) \
	  --load \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)

term:
	@echo "[term] port=$(PORT)"
	KERNEL_PATH=$$(find $(SW_DIR) -maxdepth 1 -type f -name '*.bin' | head -n1); \
	if [ -z "$${KERNEL_PATH}" ]; then \
	  echo "Error: no .bin found in $(SW_DIR)" >&2; \
	  exit 1; \
	fi; \
	$(PYTHON) -m litex.tools.litex_term $(PORT) --kernel="$${KERNEL_PATH}"

# ----------------------------
# Docs
# ----------------------------
docs:
	@echo "[docs] generating doxygen XML"
	cd $(DOC_DIR) && doxygen Doxyfile

	@echo "[docs] building Sphinx documentation"
	cd $(DOCS_DIR) && \
	LC_ALL=C.UTF-8 LANG=C.UTF-8 LANGUAGE=C.UTF-8 \
	$(MAKE) html

view-docs: docs
	@echo "[docs] opening built documentation"
	xdg-open "$(DOCS_DIR)/build/html/index.html" >/dev/null 2>&1 &

clean-docs:
	@echo "[clean-docs] removing built documentation"
	rm -rf "$(DOCS_DIR)/build"

# ----------------------------
# Cleaning
# ----------------------------
clean-sw:
	@echo "[clean-sw] removing software build outputs only"
	rm -rf "$(SW_DIR)/build"
	rm -f "$(SW_DIR)"/*.bin
	rm -f "$(SW_DIR)"/*.elf
	rm -f "$(SW_DIR)"/*.fbi

clean-sw-deep:
	@echo "[clean-sw-deep] removing software outputs + maps"
	rm -rf "$(SW_DIR)/build"
	rm -f "$(SW_DIR)"/*.bin
	rm -f "$(SW_DIR)"/*.elf
	rm -f "$(SW_DIR)"/*.fbi
	rm -f "$(SW_DIR)"/*.map

clean-gateware:
	@echo "[clean] removing gateware build output"
	rm -rf "$(BUILD_DIR)/build/$(BOARD)"

clean:
	@echo "[clean] removing build + logs + temp"
	rm -rf "$(BUILD_DIR)/build/$(BOARD)"
	rm -f "$(SERVER_PID)"
	rm -f "$(BUILD_DIR)/litex_server.log"
