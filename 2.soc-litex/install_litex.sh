#!/usr/bin/env bash
#
# install_litex.sh
#
# A script to automate installing LiteX (with Migen, LiteDRAM, LiteSPI, etc.),
# creating a Python virtual environment, and copying the target/platform files
# from the local `2.soc-litex/6.migen` folder into the LiteX installation. Supports Debian/Ubuntu
# and Arch Linux.
#
# Usage:
#   ./install_litex.sh [--litex-dir PATH] [--venv-dir PATH]
#
#   --litex-dir   Where to clone/install LiteX (default: ~/litex)
#   --venv-dir    Where to create the Python virtual environment (default: ~/litex-venv)
#
# Example:
#   ./install_litex.sh --litex-dir ~/projects/litex --venv-dir ~/projects/litex-venv
#

set -euo pipefail

### Defaults
LITEX_DIR="${HOME}/litex"
VENV_DIR="${HOME}/litex-venv"
REPOSITORY_DIR="$(pwd)"

### Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --litex-dir)
            LITEX_DIR="$2"; shift 2 ;;
        --venv-dir)
            VENV_DIR="$2"; shift 2 ;;
        --help|-h)
            echo "Usage: $0 [--litex-dir PATH] [--venv-dir PATH]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Use --help for usage."
            exit 1
            ;;
    esac
done

echo "=== LiteX Installation Script ==="
echo "LiteX will be installed to: $LITEX_DIR"
echo "Python virtualenv will be created at: $VENV_DIR"
echo "================================="

# Detect distro (Debian/Ubuntu vs Arch)
. /etc/os-release
DISTRO_ID="$ID"

install_prereqs_debian() {
    echo "--- Installing prerequisites via apt ---"
    sudo apt update
    sudo apt install -y python3 python3-venv python3-pip git build-essential \
        wget curl
}

install_prereqs_arch() {
    echo "--- Installing prerequisites via pacman ---"
    sudo pacman -Syu --noconfirm python git base-devel wget curl
}

echo
echo "Installing system prerequisites..."
case "$DISTRO_ID" in
    ubuntu|debian|linuxmint)
        install_prereqs_debian
        ;;
    arch)
        install_prereqs_arch
        ;;
    *)
        echo "Warning: Unrecognized distro '$DISTRO_ID'. Please install:"
        echo "  - Python 3, python3-venv, python3-pip, git, build-essential (gcc, make), wget, curl"
        ;;
esac

echo
echo "Creating & activating Python virtual environment..."
if [[ -d "$VENV_DIR" ]]; then
    echo "   Virtualenv directory $VENV_DIR already exists."
else
    echo "   Creating virtualenv at $VENV_DIR"
    python3 -m venv "$VENV_DIR"
fi

# Activate venv in this script context
# shellcheck source=/dev/null
source "${VENV_DIR}/bin/activate"

echo "   Upgrading pip inside venv..."
pip install --upgrade pip setuptools wheel

echo
echo "Cloning/fetching LiteX setup script..."
mkdir -p "$LITEX_DIR"
cd "$LITEX_DIR"

if [[ -f litex_setup.py ]]; then
    echo "   litex_setup.py already exists. Pulling latest."
    rm -f litex_setup.py
fi

wget -q https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
chmod +x litex_setup.py

echo
echo "Running LiteX installer (Migen, LiteDRAM, LiteSPI, LiteEth, LiteScope, etc.)..."
echo "   This may take several minutes..."
python3 litex_setup.py --init --install --gcc=riscv --config=full

echo
echo "5) Copying target/platform files from local 2.soc-litex/6.migen to LiteX boards..."
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
SOURCE_MIGEN_DIR="${SCRIPT_DIR}/6.migen"
LITEX_BOARDS_DIR="${LITEX_DIR}/litex-boards/litex_boards"

if [[ ! -d "$SOURCE_MIGEN_DIR" ]]; then
    echo "Error: '2.soc-litex/6.migen' folder not found under script directory: $SOURCE_MIGEN_DIR"
    echo "Make sure you run this script from the root of the repository containing '2.soc-litex/6.migen'."
    exit 1
fi

if [[ ! -d "$LITEX_BOARDS_DIR" ]]; then
    echo "Error: Litex boards directory not found at expected location: $LITEX_BOARDS_DIR"
    echo "Did litex_setup.py complete successfully?"
    exit 1
fi

echo "   Copying contents of $SOURCE_MIGEN_DIR into $LITEX_BOARDS_DIR"
cp -r "${SOURCE_MIGEN_DIR}/." "$LITEX_BOARDS_DIR/"

echo
echo "=== Installation Complete ==="
echo "To use LiteX, do:"
echo "  source \"$VENV_DIR/bin/activate\""
echo "  cd \"$LITEX_DIR\""
echo "  # Now you can 'litex_boards' or any LiteX commands"
echo
echo "The new target/platform files from '2.soc-litex/6.migen' are available under:"
echo "  $LITEX_BOARDS_DIR"
