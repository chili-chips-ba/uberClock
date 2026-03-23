from __future__ import annotations

from pathlib import Path
from typing import Iterable


_THIS_FILE = Path(__file__).resolve()
_REPO_ROOT = _THIS_FILE.parents[3]
_RTL_DIR = _REPO_ROOT / "1.hw"


def rtl_dir() -> Path:
    """Return the fixed RTL root directory for this repository."""
    return _RTL_DIR


def add_sources(platform, rel_files: Iterable[str], base_dir: Path | None = None) -> None:
    """
    Add RTL files to a LiteX platform.

    `rel_files` are paths relative to `base_dir` or, by default, to `1.hw/`.
    """
    root_dir = _RTL_DIR if base_dir is None else Path(base_dir).resolve()

    for rel_path in rel_files:
        platform.add_source(str((root_dir / rel_path).resolve()))
