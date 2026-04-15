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
DEFAULT_VENV  := $(firstword $(wildcard $(REPO_ROOT)/.venv $(HOME)/litex-venv))
PYTHON        ?= $(if $(VIRTUAL_ENV),$(VIRTUAL_ENV)/bin/python3,$(if $(DEFAULT_VENV),$(DEFAULT_VENV)/bin/python3,python3))
PYTHONPATH_REPO ?= $(PYTHON_PKG_DIR)/src
SOC_MODULE    ?= uberclock_soc

# Board/target
BOARD         := alinx_ax7203

# Defaults
FREQ          ?= 100e6
OPTIONS       ?= --with-uberddr3 --with-uberclock --with-ethernet

SERVER_PID    := $(BUILD_DIR)/litex_server.pid
CSR_CSV_FILE  ?= $(BUILD_DIR)/build/$(BOARD)/csr.csv

.PHONY: help build-board build-sw load term \
        clean clean-gateware clean-sw clean-sw-deep \
        install-python print-python docs view-docs clean-docs

help:
	@echo "Targets:"
	@echo "  make build-board [FREQ=100e6] [OPTIONS='...']"
	@echo "  make load        [FREQ=100e6] [OPTIONS='...']"
	@echo "  make term        [PORT=/dev/ttyUSB0]"
	@echo "  make build-sw    [DEMO_FLAGS='..']"
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
	@echo "[build] sys_clk=$(FREQ) options=$(OPTIONS)"
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) -m $(SOC_MODULE) \
	  --build \
	  --sys-clk-freq=$(FREQ) \
	  $(OPTIONS)

# ----------------------------
# Build software under 2.sw/
# ----------------------------
build-sw:
	@echo "[software build] DEMO_FLAGS='$(DEMO_FLAGS)'"
	@echo "→ Executing demo.py in $(SW_DIR)"
	cd $(SW_DIR) && \
	PYTHONPATH=$(PYTHONPATH_REPO):$$PYTHONPATH \
	$(PYTHON) ./demo.py \
	  --build-path=$(BUILD_DIR)/build/$(BOARD) \
	  --mem=main_ram \
	  --data-mem=main_ram \
	  $(DEMO_FLAGS)

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
