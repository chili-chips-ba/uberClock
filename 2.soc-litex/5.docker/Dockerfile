# Base stage for common tools and dependencies
FROM ubuntu:22.04 AS base

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/root/.cargo/bin:/usr/local/bin:$PATH"

# Install common dependencies
RUN apt-get update && apt-get install --no-install-recommends -y \
    autoconf bc bison build-essential ca-certificates ccache flex file git help2man \
    libfl2 libfl-dev libgoogle-perftools-dev numactl perl python3 python3-pip zlib1g python3-venv \
    wget curl tar openocd meson gperf desktop-file-utils libgtk-3-dev libbz2-dev \
    libjudy-dev libgirepository1.0-dev libgtk-4-dev gnupg libgmp-dev netbase \
    software-properties-common libssl-dev cargo clang lld libreadline-dev gawk \
    tcl-dev libffi-dev graphviz xdot sudo pkg-config libboost-system-dev \
    libboost-python-dev libboost-filesystem-dev texinfo && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Install Python tools
RUN python3 -m pip install meson ninja peakrdl

#======================================================#
#                       Verible                        #
#======================================================#
RUN mkdir -p /opt/verible && \
    wget -O /opt/verible/verible.tar.gz "https://github.com/chipsalliance/verible/releases/download/v0.0-3860-gf3da2ce6/verible-v0.0-3860-gf3da2ce6-linux-static-x86_64.tar.gz" && \
    tar -xzf /opt/verible/verible.tar.gz -C /opt/verible --strip-components=1 && \
    ln -s /opt/verible/bin/* /usr/local/bin && \
    rm /opt/verible/verible.tar.gz

# ===== Verilator stage ===== #
FROM base AS verilator

#======================================================#
#                       Verilator                      #
#======================================================#
RUN git clone --depth 1 https://github.com/verilator/verilator.git && \
    cd verilator && \
    autoconf && ./configure && \
    make -j$(nproc) && make install && \
    rm -rf /verilator

# ==== Yosys stage ==== #
FROM base AS yosys

#======================================================#
#                        yosys                         #
#======================================================#
RUN git clone --depth 1 https://github.com/YosysHQ/yosys.git && \
    cd yosys && \
    make config-clang && git submodule update --init && \
    make -j$(nproc) && make install && \
    rm -rf /yosys

# ===== Final stage combining everything ===== #
FROM base AS final

# Copy Verilator and Yosys from their respective stages
COPY --from=verilator /usr/local /usr/local
COPY --from=yosys /usr/local /usr/local

# -------------------------------------------------------------------
#  LiteX + ecosystem installation
# -------------------------------------------------------------------
# create a venv under /opt/litex-venv
ENV LITEX_VENV=/opt/litex-venv
RUN python3 -m venv ${LITEX_VENV}

# ensure venv’s bin directory is on PATH
ENV PATH="${LITEX_VENV}/bin:${PATH}"

RUN mkdir -p /opt/litex && \
    cd /opt/litex && \
    wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py && \
    chmod +x litex_setup.py && \
    /opt/litex-venv/bin/python3 litex_setup.py --init --install --gcc=riscv --config=full

# Additional repositories for FPGA and memory modeling
RUN git config --global http.postBuffer 1048576000 && \
    git config --global core.compression 0 && \
    git config --global http.lowSpeedLimit 0 && \
    git config --global http.lowSpeedTime 999 && \
    git clone https://github.com/chili-chips-ba/uberClock.git /uberclock


#======================================================#
#                         sv2v                         #
#======================================================#
RUN curl -sSL https://get.haskellstack.org/ | sh -s - -f && \
    git clone --depth 1 https://github.com/zachjs/sv2v.git && \
    cd sv2v && make

#======================================================#
#                       surfer                         #
#======================================================#
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | bash -s -- -y && \
    rustup update && \
    cargo install --git https://gitlab.com/surfer-project/surfer surfer


# Set the working directory
WORKDIR /workspace
