# Installation Guide

## LiteX Installation

Follow these steps to get a clean Python environment and install LiteX (with Migen, LiteDRAM, LiteSPI, etc.) on Linux or WSL.
Afterwards an explanation will be given on how to incorporate the design correctly.

### 1. Install system prerequisites
#### Arch Linux

```
sudo pacman -Syu python git base-devel
```

#### Debian/Ubuntu
```
sudo apt update
sudo apt install python3 python3-venv python3-pip git build-essential
```

### 2. Create & activate a virtual environment

1. Create a venv (e.g. ~/litex-venv):
```
python3 -m venv ~/litex-venv
```

2. Activate it:

```
source ~/litex-venv/bin/activate
```

### 3. Install LiteX & its cores

A more detailed explanation of the installation can be found on the [official Litex github](https://github.com/enjoy-digital/litex/wiki/Installation).

1. Make a directory for the installer:

```
mkdir -p ~/litex
cd ~/litex
```

2. Fetch the official setup script and make it executable:
```
wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
chmod +x litex_setup.py
```
Install Python 3.6+ and FPGA vendor's development tools and/or Verilator.
Install Migen/LiteX and the LiteX's cores:

3. Run the installer:

```
./litex_setup.py --init --install --gcc=riscv --config=full
```

This installs Migen, LiteX, LiteDRAM, LiteSPI, LiteEth, LiteScope, etc., into your venv.
If you're building a CPU-based SoC and need a RISC-V GCC add the `--gcc=riscv` flag, otherwise you can emit it. Here we will use it.

4. Copy the target and platform file in your litex installation folder

Until the Pull Request is approved you will need to manually copy the target and platform file for the specific board we are using. In this case it is the `alinx_ax7203` board and the files are located in the `6.migen` folder of the repository. 

### Use a script that automates all of this

If you'd like to use a bash script that does all of the steps above you can simply use the [install_litex.sh](https://github.com/chili-chips-ba/uberClock/blob/main/2.soc-litex/install_litex.sh) script located in the [2.soc-litex](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex) folder.
