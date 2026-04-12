#!/usr/bin/env bash
# setup_dev.sh
#
# LiteX development environment bootstrap using pyenv + Python 3.11
#
# What this does:
#   - installs pyenv (if missing)
#   - installs Python 3.11 via pyenv
#   - creates a virtualenv at a fixed directory
#   - installs LiteX into that virtualenv
#
# Usage:
#   ./setup_dev.sh [--litex-dir PATH] [--venv-dir PATH] [--no-gcc]
#
# Defaults:
#   --litex-dir  ~/litex
#   --venv-dir   ~/litex-venv
#
set -euo pipefail

LITEX_DIR="${HOME}/litex"
VENV_DIR="${HOME}/litex-venv"
PYTHON_VERSION="3.11.8"
NO_GCC=0

usage() {
  echo "Usage: $0 [--litex-dir PATH] [--venv-dir PATH] [--no-gcc]"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --litex-dir) LITEX_DIR="$2"; shift 2 ;;
    --venv-dir)  VENV_DIR="$2"; shift 2 ;;
    --no-gcc)    NO_GCC=1; shift ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "Unknown argument: $1"; usage; exit 1 ;;
  esac
done

echo "=== LiteX Dev Setup (pyenv + venv dir) ==="
echo "Python version : ${PYTHON_VERSION}"
echo "Venv dir       : ${VENV_DIR}"
echo "LiteX dir      : ${LITEX_DIR}"
echo "========================================="

# ---------------------------------------------------------------------------
# System prerequisites for pyenv
# ---------------------------------------------------------------------------
install_prereqs() {
  if [[ -r /etc/os-release ]]; then
    . /etc/os-release
    case "${ID:-}" in
      ubuntu|debian|linuxmint)
        sudo apt update
        sudo apt install -y \
          git curl build-essential \
          libssl-dev zlib1g-dev libbz2-dev \
          libreadline-dev libsqlite3-dev \
          libffi-dev liblzma-dev tk-dev \
          xz-utils wget
        return
        ;;
      arch)
        sudo pacman -Syu --noconfirm \
          git base-devel openssl zlib xz tk
        return
        ;;
    esac
  fi

  echo "WARNING: Unknown distro. Ensure pyenv build deps are installed."
}

install_prereqs

# ---------------------------------------------------------------------------
# Install pyenv if missing
# ---------------------------------------------------------------------------
if ! command -v pyenv >/dev/null 2>&1; then
  echo "--- Installing pyenv ---"
  curl https://pyenv.run | bash
fi

export PYENV_ROOT="${HOME}/.pyenv"
export PATH="${PYENV_ROOT}/bin:${PATH}"
eval "$(pyenv init -)"

# ---------------------------------------------------------------------------
# Install Python 3.11 via pyenv
# ---------------------------------------------------------------------------
if ! pyenv versions --bare | grep -q "^${PYTHON_VERSION}$"; then
  echo "--- Installing Python ${PYTHON_VERSION} ---"
  pyenv install "${PYTHON_VERSION}"
fi

PYENV_PYTHON="${PYENV_ROOT}/versions/${PYTHON_VERSION}/bin/python"

# ---------------------------------------------------------------------------
# Create virtualenv at fixed directory
# ---------------------------------------------------------------------------
echo "--- Creating virtualenv ---"
if [[ ! -d "${VENV_DIR}" ]]; then
  "${PYENV_PYTHON}" -m venv "${VENV_DIR}"
fi

# shellcheck source=/dev/null
source "${VENV_DIR}/bin/activate"

python -V
python -m pip install -U pip setuptools wheel

# ---------------------------------------------------------------------------
# Install LiteX
# ---------------------------------------------------------------------------
echo "--- Installing LiteX ---"
mkdir -p "${LITEX_DIR}"
cd "${LITEX_DIR}"

rm -f litex_setup.py
wget -q https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
chmod +x litex_setup.py

GCC_ARGS=()
if [[ "${NO_GCC}" -eq 0 ]]; then
  GCC_ARGS+=(--gcc=riscv)
fi

python litex_setup.py --init --install --config=full "${GCC_ARGS[@]}"

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
echo
echo "=== Done ==="
echo "Activate environment:"
echo "  source \"${VENV_DIR}/bin/activate\""
echo
echo "Sanity check:"
echo "  python -c \"import litex, migen; print('LiteX OK')\""
