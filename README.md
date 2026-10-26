# uberClock


[![NLnet Foundation](https://img.shields.io/badge/funded%20by-NLnet%20Foundation-74AA00)](https://nlnet.nl/)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://chili-chips-ba.github.io/uberClock/)
[![GitHub stars](https://img.shields.io/github/stars/uberClock?style=social)](https://github.com/uberClock/stargazers)

---
Clocks can be extracted from GPS satellite signals, or locally generated with MEMS oscillators, SAW resonators, quartz crystal (XTAL, XO) or piezo resonators, often set in “ovens” (TCXO, OCXO), derived from atomic properties (like Cesium Beam, Hydrogen Maser, Rubidium, Strontium or Ytterbium), or obtained in another way.

<p align="center">
  <img width=250 src="0.doc/artwork/uberClock.sticker.png">
</p>

They differ in Absolute Accuracy, Long-term Frequency Stability (e.g. due to aging), Short-term Frequency Stability (due to temperature changes), Phase Noise (aka Jitter), physical size, complexity, immunity to external interference (due to physical vibrations, humidity, EMP, EW), power consumption, cost, etc. These differences are categorized as “Clock Strata”, whereby a clock source must meet a standardized set of requirements for each Stratum level.

> This work is about researching and exploiting the properties of multi-mode crystal oscillators in order to achieve stability comparable to a Stratum 2 Rubidium clock, all at a fraction of the total cost of ownership. We plan on collecting large empirical datasets, constructing experimental prototypes, and using DSP / numerical methods to mitigate (1) temperature variations, (2) dynamic acceleration and (3) static gravity effects. The project aims for XTAL frequency stability by means of numerous mathematical calculations performed in FPGA, using open-source tools, including CflexHDL+PipelineC HLS flow.

This is a Proof-of-Concept (PoC) and stepping stone for future applied research projects on this theme, possibly extending into the field of Artificial Intelligence. In addition to a working prototype (PCBs, FPGA Gateware and Embedded Firmware), the project will deliver a series of scientific papers.

#### References
- [Unleashing the Mystery of Crystal Cuts](https://xoprof.com/2023/09/unleashing-the-mystery-of-crystal-cuts)
- [Quartz Crystal Cuts: AT, BT, SC, CT](https://www.electronics-notes.com/articles/electronic_components/quartz-crystal-xtal/crystal-resonator-cuts-at-bt-sc-ct.php)
- [It's All about the Angle - The AT-Cut for Quartz Crystals](https://www.jauch.com/blog/en/its-all-about-the-angle-the-at-cut-for-quartz-crystals)
- [Oscillator aging and its importance in precision timing](https://www.sitime.com/company/newsroom/articles/oscillator-aging-and-its-importance-precision-timing)

- [Python tool for filter design, with GUI](https://github.com/chipmuenk/pyfda) with [video demo](https://www.youtube.com/watch?v=IDKKr-ry9tc)

- [DSP Filters in AmaranthHDL](https://github.com/amaranth-farm/amlib/tree/main/amlib/dsp)
- [DSP Filters in Verilog](https://github.com/ZipCPU/dspfilters)
- [Papers](0.doc/Quartz/papers)


--------------------

## Hardware platform
- **Physics Package** -- Full-custom [Analog Board](https://github.com/jdbrinton/uberclock) with multi-mode Quartz Crystal

<p align="center">
  <img width=250 src="0.doc/Quartz/Analog-Card.1.jpg">
  <img width=200 src="0.doc/Quartz/Analog-Card.2.png">
</p>
  
- [AX7203 Artix7-200 FPGA Board](https://www.en.alinx.com/Product/FPGA-Development-Boards/Artix-7/AX7203.html)
<p align="center">
  <img width=600 src="0.doc/Alinx/FPGA-Board--Artix7-200--AX7203.jpg">
</p>
  
- [AN9238 2xADC,  65MSPS, 12bit](https://www.en.alinx.com/Product/Add-on-Modules/AN9238.html)
<p align="center">
    <img width=300 src="0.doc/Alinx/2xADC--65MSPS-12bit--AN9238.jpg">
</p>
    
- [AN9767 2xDAC, 125MSPS, 14bit](https://www.en.alinx.com/Product/Add-on-Modules/AN9767.html)
<p align="center">
  <img width=300 src="0.doc/Alinx/2xDAC--125MSPS-14bit--AN9767.jpg">
</p>


--------------------

# Project Status

#### 1. Acquisition of Hardware Platform
 - [x] Design, manufacture and debug the "Physics Package" card.
 - [x] Procure and distribute FPGA, ADC and DAC cards.

#### 2. Digital Infrastructure
 - [x] Familiarize with ALINX boards.
    - Toggle LEDs
    - Write RTL for interfaces to ADC and DAC chips
    - Write RTL to test their operation
    - Perform this testing. Debug and fix the problems as they arise
 - [x] Create CPU hardware subsystem based on an open-source RISC-V core, memories, UART and debug port. 
 - [x] Create a bare-metal software skeleton, as the foundation for writing future DSP applications. Create and test software build flow.
 - [x] Test operation of CPU subsystem. Profile its performance. 
 - [x] Map ADCs and DACs into CPU memory space and test SW communication with them.
 
#### 3. DSP Model and Documentation
 - [ ] Model quartz crystal and DSP datapath in C or Python.
 - [ ] Create _Theory of Operation_ document with explanation of concepts, tradeoffs and criteria used to devise solutions. 
 - [ ] Post the _Executive Summary_ here.

#### 4. Integration and Characterization
 - [ ] Bring up the complete system with digital and analog card connected to each other.
 - [ ] Perform manual characterization of individual crystals.
 - [ ] Develop a semi or fully automated crystal characterization procedure.
 
#### 5.  Implementation of DSP algorithms
 - [ ] Implement HW side of DSP algorithm on FPGA.
 - [ ] Implement SW side of DSP algorithm in the RISC-V CPU.
 - [ ] Integrate DSP hardware and software into a complete system.
 
#### 6. Benchmarking
 - [ ] Test the DSP together with crystal.
    - we need a reliable reference clock source for this, preferably Stratum 0
    - with a good Spectrum Analyzer
 - [ ] Fine-tune DSP algorithm based on the obtained measurements.
 - [ ] Conducting additional experiments with the corrected DSP algorithm (in simulation and on hardware).
 
#### 7. Port from Vivado to openXC7
 - [x] CPU + UART + integrated RAM builds and routes end-to-end with openXC7 1.0.0
   (Yosys + nextpnr-himbaechel + prjxray), no Vivado involved; meets 100 MHz with seed 1.
 - [ ] `--with-uberclock` (the DSP core) does not build under openXC7 yet: it hits an
   upstream nextpnr placement-legality bug, still present in openXC7 1.0.0. See
   [Building with openXC7](#building-with-openxc7-vivado-free-toolchain) below.
 - [ ] `--with-uberddr3`, `--with-ethernet`, `--with-hdmi` are not expected to work
   under openXC7 regardless (Vivado-only I/O constraints and primitives); untested.

--------------------

# Building with openXC7 (Vivado-free toolchain)

Besides Vivado, the SoC can be built with
[openXC7](https://github.com/openXC7/toolchain-installer) **1.0.0**, a fully
open-source flow for the Alinx AX7203 (`xc7a200tfbg484-2`):
Yosys 0.69 → `nextpnr-himbaechel` ([openXC7/nextpnr](https://github.com/openXC7/nextpnr)
1.0.0) → prjxray `fasm2frames` / `xc7frames2bit`. LiteX drives it through
`XilinxYosysNextpnrToolchain` (`TOOLCHAIN=openxc7`).

The full debugging log (every error, its cause, and its fix) is
[`docs/openxc7_port/main.tex`](docs/openxc7_port/main.tex); the openXC7 1.0.0
section there is the current reference.

## What works

| Configuration | Result |
|---|---|
| RISC-V CPU + UART + 64 KiB integrated RAM | ✅ Builds a `.bit` in 1–2 min; seed 1 meets 100 MHz timing |
| `--with-uberclock` (DSP core) | ❌ nextpnr placement-legality bug (see below) |
| `--with-uberddr3`, `--with-ethernet`, `--with-hdmi` | ⚠️ Untested; expected to need Vivado |

The 1.0.0 bitstreams have not been tested on hardware yet.

## Install and build, step by step

Tested on Ubuntu. Run from the repository root.

**1. LiteX environment** (creates `~/litex-venv`, installs `fasm2frames`):
```bash
2.soc/setup_dev.sh --with-openxc7
```
Add `--no-gcc` if a RISC-V GCC is already installed or you have no `sudo`;
otherwise the script stops at the failing `apt install` before it gets to the
openXC7 step.

**2. Copy the project's board file into litex-boards** (defines the ADC/DAC
pins and accepts `toolchain=`; repeat after every `litex_setup.py --update`):
```bash
cp 2.soc/6.migen/platforms/alinx_ax7203.py \
   ~/litex/litex-boards/litex_boards/platforms/alinx_ax7203.py
```

**3. Build openXC7 1.0.0 from source:**
```bash
git clone --branch 1.0.0 https://github.com/openXC7/toolchain-installer
cd toolchain-installer
./toolchain-sources-builder.sh all     # installs into /opt/openxc7
# without sudo: INSTALL_PREFIX=$HOME/opt/openxc7-1.0 ./toolchain-sources-builder.sh all
```

**4. Generate the chip database once** (LiteX can't do this itself with 1.0.0;
about 2.5 min and 7 GB RAM):
```bash
P=/opt/openxc7                         # or your INSTALL_PREFIX
source $P/export.sh                    # puts the 1.0.0 python3/bbasm first on PATH
mkdir -p ~/openxc7-chipdb
python3 $P/share/nextpnr/himbaechel/uarch/xilinx/gen/xilinx_gen.py \
  --xray $P/share/nextpnr/prjxray-db/artix7 \
  --device xc7a200t --bba /tmp/xc7a200t.bba
bbasm -l /tmp/xc7a200t.bba ~/openxc7-chipdb/xc7a200tfbg484.bin
rm /tmp/xc7a200t.bba
```
`--device` is the fabric (`xc7a200t`), not the part. The output must be
named `xc7a200tfbg484.bin`.

**5. Build:**
```bash
source ~/litex-venv/bin/activate
cd 3.build
make build-board TOOLCHAIN=openxc7 \
  OPTIONS='--integrated-main-ram-size=0x10000 --nextpnr-seed=1'
```
For a custom install prefix or chipdb folder, add
`OPENXC7_DIR=$HOME/opt/openxc7-1.0 CHIPDB=...`. In zsh, pass these as separate
words (or `export` them); `make build-board $VARS` with all of them in one
variable does not work there.

**6. Check timing before using the bitstream.** LiteX lets nextpnr write a
bitstream even when timing fails, so look at the last
`Max frequency for clock 's7mmcm0_clkout_buf'` line in the output. If it says
`FAIL`, try another `--nextpnr-seed` or lower the clock with `FREQ=90e6`. The
bitstream is `3.build/build/alinx_ax7203/gateware/alinx_ax7203.bit`
(`make load` to program it).

Builds regenerate tracked files under `3.build/build/alinx_ax7203/`; run
`git checkout -- 3.build/build` before committing.

## What was changed to make it work

- `soc.py`: the platform is built by `_make_platform()`, which also works with
  stock litex-boards (for CPU-only builds) and uses the dash-free part name
  `xc7a200tfbg484-2` that prjxray-db, the chipdb and nextpnr expect.
- `soc.py` / `clocking.py`: `IDELAYCTRL` is only instantiated when DDR3 or
  Ethernet uses I/ODELAYs; nextpnr-himbaechel rejects an unused one.
- `soc.py`: the `OPENXC7_PLACER` override now actually reaches nextpnr (current
  LiteX passes no `--placer`, so the old text replacement did nothing).
- `3.build/Makefile`: openXC7 paths follow the 1.0.0 install layout.

## Known problems

- **`--with-uberclock` does not build.** Synthesis completes (70% LUTs, 48%
  CARRY4, so not a capacity issue), but nextpnr cannot legally place a CARRY4
  from a CORDIC stage adder (`Unable to find legal placement for cell
  '...cordic_stages[17]...carry4...' of type 'CARRY4'`). `OPENXC7_NOCARRY=1`
  moves the same failure to a flip-flop cell, and `OPENXC7_PLACER=sa` stalls
  in initial placement. This is the same upstream bug the old nextpnr-xilinx
  had; openXC7 1.0.0 doesn't fix it.
- **Timing at 100 MHz is marginal.** Sys-clock Fmax across seeds was
  86–107 MHz; only seed 1 passed. The critical path is inside VexRiscv and
  mostly routing.
- **64 KiB RAM is required.** The default 256 KiB RAM made the old toolchain
  mis-place a cascaded BRAM pair; this hasn't been re-tested with 1.0.0.
- **`--with-uberddr3`, `--with-ethernet`, `--with-hdmi`** are expected to need
  Vivado: LiteX drops Vivado-only I/O properties such as `INTERNAL_VREF` for
  this toolchain, and there is no false-path support for the 400 MHz DDR3 PHY.

**Use Vivado for any build that includes `--with-uberclock`, `--with-uberddr3`,
`--with-ethernet`, or `--with-hdmi`.**

--------------------

# DSP Theory of Operation
- WIP

# Bit-accurate Models
## Multi-mode Quartz Crystal
## DSP datapath
- WIP

# Bit-accurate Sim of the entire algorithm
- WIP
  
--------------------

# HW Architecture
- WIP
  
<p align="center">
  <img width=800 src="0.doc/HW_architecture.png">
</p>


--------------------

# SW Architecture
- WIP


--------------------

# License

This repository uses multiple licenses. Text and documentation are licensed under CC-BY-SA-4.0. LiteX integration code is licensed under BSD-2-Clause. Other original code is licensed under GPL-3.0-or-later. Existing third-party or separately licensed files keep their own licenses, including BSD-3-Clause, Apache-2.0 for ZipCPU/WB2AXIP code, and ISC for PicoRV32. See [LICENSE](LICENSE) for the full license texts.


--------------------

## Acknowledgements
We are grateful to **NLnet Foundation** for their sponsorship of this development activity.

<p align="center">
   <img width="25%" src="https://nlnet.nl/logo/banner.svg">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
   <img width="25%" src="https://nlnet.nl/image/logos/NGI0Core_tag.svg">
</p>

This project was funded through the NGI0 Commons Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement N<sup>o</sup> 101135429. Additional funding is made available by the Swiss State Secretariat for Education, Research and Innovation (SERI).

### Public posts:
- [2025-05-25](https://www.linkedin.com/posts/adc-dac-riscv-activity-7332575086975078400-41q6/?utm_source=share&utm_medium=member_desktop&rcm=ACoAAAJv-TcBSi_5ff0VNMrInrT-xg44YF3jnyU)
- [2025-02-10](https://www.linkedin.com/posts/dsp-adc-cordic-activity-7294943218167689216-ZzOs?utm_source=share&utm_medium=member_desktop)


--------------------
#### End of Document
