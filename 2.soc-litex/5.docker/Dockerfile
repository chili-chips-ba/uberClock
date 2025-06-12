FROM ubuntu:22.04

RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://us.archive.ubuntu.com/ubuntu|g' /etc/apt/sources.list

# ──────────────────────────────────────────────────────────────────────────
# Environment variables
# ──────────────────────────────────────────────────────────────────────────
ENV DEBIAN_FRONTEND=noninteractive
ENV LITEX_VENV=/opt/litex-venv
ENV PATH="${LITEX_VENV}/bin:/usr/local/bin:/root/.cargo/bin:${PATH}"
ENV LITEX_PATH=/opt/litex
ENV REPOSITORY_DIR=/uberClock

ENV XILINX_INSTALL_DIR=/opt/Xilinx
ENV PATH="${PATH}:${XILINX_INSTALL_DIR}/Vivado/2024.2/bin"

# ──────────────────────────────────────────────────────────────────────────
# Install system dependencies, editors, and RISC-V cross-toolchain
# ──────────────────────────────────────────────────────────────────────────
RUN apt-get update && apt-get install --no-install-recommends -y \
    # Editors & basic tools
    nano gedit \
    # Build tools, Python, Git, etc.
    autoconf bc bison build-essential ca-certificates ccache flex file git help2man \
    libfl2 libfl-dev libgoogle-perftools-dev numactl perl python3 python3-pip python3-venv \
    wget curl tar openocd meson gperf desktop-file-utils libgtk-3-dev libbz2-dev \
    libjudy-dev libgirepository1.0-dev libgtk-4-dev gnupg libgmp-dev netbase \
    software-properties-common libssl-dev cargo clang lld libreadline-dev gawk \
    tcl-dev libffi-dev graphviz xdot sudo pkg-config libboost-system-dev \
    libboost-python-dev libboost-filesystem-dev texinfo \
    # 64-bit RISC-V cross-compiler
    gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# ──────────────────────────────────────────────────────────────────────────
# Install Python tools (system-wide)
# ──────────────────────────────────────────────────────────────────────────
RUN python3 -m pip install --no-cache-dir meson ninja peakrdl

# ──────────────────────────────────────────────────────────────────────────
# Build and install Verilator
# ──────────────────────────────────────────────────────────────────────────
RUN git clone --depth 1 https://github.com/verilator/verilator.git /verilator && \
    cd /verilator && autoconf && ./configure && make -j"$(nproc)" && make install && \
    rm -rf /verilator

# ──────────────────────────────────────────────────────────────────────────
# Create LiteX virtual environment and install LiteX ecosystem
# ──────────────────────────────────────────────────────────────────────────
RUN python3 -m venv ${LITEX_VENV} && \
    mkdir -p ${LITEX_PATH} && \
    cd ${LITEX_PATH} && \
    wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py && \
    chmod +x litex_setup.py && \
    ${LITEX_VENV}/bin/python3 litex_setup.py --init --install --config=full \
    ${LITEX_VENV}/bin/python3 litex_setup.py --gcc=riscv

# ──────────────────────────────────────────────────────────────────────────
# Clone uberClock repository and merge 6.migen into LiteX boards
# ──────────────────────────────────────────────────────────────────────────
RUN git config --global http.postBuffer 1048576000 && \
    git config --global core.compression 0 && \
    git config --global http.lowSpeedLimit 0 && \
    git config --global http.lowSpeedTime 999 && \
    git clone https://github.com/chili-chips-ba/uberClock.git ${REPOSITORY_DIR} && \
    cp -r ${REPOSITORY_DIR}/2.soc-litex/6.migen/* ${LITEX_PATH}/litex-boards/litex_boards/

# ──────────────────────────────────────────────────────────────────────────
# Copy Vivado 2024.2 WebPACK installer into image and install silently
# ──────────────────────────────────────────────────────────────────────────
COPY Xilinx_Vivado_2024.2_WebPACK.tar.gz /tmp/

RUN mkdir -p ${XILINX_INSTALL_DIR} && \
    cd /tmp && \
    tar -xzf Xilinx_Vivado_2024.2_WebPACK.tar.gz -C /tmp && \
    /tmp/Xilinx_Vivado_2024.2_WebPACK/xsetup \
      --batch Install \
      --agree XilinxEULA,3rdPartyEULA \
      --install_dir ${XILINX_INSTALL_DIR}/Vivado/2024.2 \
      --product Vivado \
      --partition all \
      --verbose \
      --components VivadoHLx:Vivado && \
    rm -rf /tmp/Xilinx_Vivado_2024.2_WebPACK*

# ──────────────────────────────────────────────────────────────────────────
# Working directory
# ──────────────────────────────────────────────────────────────────────────
WORKDIR /workspace
