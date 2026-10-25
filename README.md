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
 - [x] CPU + UART + integrated RAM builds and routes end-to-end with openXC7
   (Yosys + nextpnr-xilinx + prjxray), no Vivado involved.
 - [ ] `--with-uberclock` (the DSP core) does not build cleanly under openXC7 yet:
   it hits upstream nextpnr-xilinx placement-legality bugs. See
   [Building with openXC7](#building-with-openxc7-vivado-free-toolchain) below.
 - [ ] `--with-uberddr3`, `--with-ethernet`, `--with-hdmi` are not expected to work
   under openXC7 regardless (Vivado-only I/O constraints and primitives); untested.

--------------------

# Building with openXC7 (Vivado-free toolchain)

In addition to Vivado, this project can be built with
[openXC7](https://github.com/openXC7/toolchain-installer) -- a fully open-source
Yosys → nextpnr-xilinx → prjxray flow -- for the Alinx AX7203
(`xc7a200tfbg484-2`). LiteX already supports this via its
`XilinxYosysNextpnrToolchain`; no RTL changes were needed for the CPU-only
configuration below, beyond a few pre-existing bugs (unrelated to the
toolchain choice) that had to be fixed first -- see
[`docs/openxc7_port/main.tex`](docs/openxc7_port/main.tex) for the full,
chronological debugging log, root causes, and fixes.

## What works

RISC-V CPU + UART + 64 KiB integrated RAM builds and routes to a working
`.bit` file, fully through the open flow:

```bash
cd 3.build
make build-board TOOLCHAIN=openxc7 \
  OPTIONS='--integrated-main-ram-size=0x10000 --nextpnr-seed=42'
```

This requires an openXC7 install (tested with the toolchain-installer layout
under `/opt/openxc7`) and a pre-generated chip database for
`xc7a200tfbg484`. Run `2.soc/setup_dev.sh --with-openxc7` once to also install
`fasm2frames` (from `f4pga/prjxray`), which the openXC7 packer stage needs but
which the toolchain-installer binary install does not ship.

The 64 KiB RAM size is a required workaround, not a free choice: the default
256 KiB integrated RAM makes Yosys infer a depth-cascaded BRAM pair, and
nextpnr-xilinx does not enforce the cascade pair's column-adjacency
constraint, producing an unroutable placement. 64 KiB avoids BRAM cascading
entirely.

## What doesn't work, and why

`--with-uberclock` (the 11-channel DSP core) does not build cleanly under
openXC7. Synthesis completes (after disabling LiteX's default `-flatten`
before optimization, and switching to Yosys's `abc9` LUT mapper to stay
within LUT capacity), but placement then hits a genuine, reproducible
**nextpnr-xilinx placement-legality bug**: its placer cannot legalize a
CARRY4 chain shape produced by Yosys abc9's ALU-extraction pass for the
CORDIC pipeline's adders. Every placer/option combination tried (`heap`;
`heap` with `-nocarry`; `sa`) hits an independent variant of this bug class,
with one signature reproduced on two unrelated netlists. This is a toolchain
maturity gap, not something fixable from this project's RTL or LiteX
integration -- see `docs/openxc7_port/main.tex` for the full bisection and
the three distinct bug signatures found.

`--with-uberddr3`, `--with-ethernet`, and `--with-hdmi` are expected to need
Vivado regardless of the above: LiteX silently drops the Vivado-only I/O
properties UberDDR3's PHY needs (`INTERNAL_VREF`, etc.) under this toolchain,
and `XilinxYosysNextpnrToolchain` has no real static timing analysis or false-
path support, so a 400 MHz DDR3 PHY domain would be effectively unconstrained.

**Until the upstream nextpnr-xilinx bugs are fixed, use Vivado for any build
that includes `--with-uberclock`, `--with-uberddr3`, `--with-ethernet`, or
`--with-hdmi`.** openXC7 is solid for the CPU-only bring-up case above, and
for any future peripheral-free RTL work where a fully open flow matters.

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
