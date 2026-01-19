from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


def _find_rtl_root(start: Path) -> Path:
    """Walk upward from `start` and return the first directory that contains `1.hw/`."""
    for parent in [start, *start.parents]:
        hw_dir = parent / "1.hw"
        if hw_dir.is_dir():
            return hw_dir
    raise RuntimeError(
        "Could not locate RTL directory '1.hw/'.\n"
        f"Search start: {start}\n"
        "Expected layout:\n"
        "  <repo-root>/1.hw/\n"
        "  <repo-root>/8.python/src/uberclock_soc/\n"
    )


_THIS_FILE = Path(__file__).resolve()


@dataclass(frozen=True)
class RtlLayout:
    """Resolved RTL layout for this repo."""
    hw_dir: Path
    repo_root: Path


def rtl_layout() -> RtlLayout:
    """Return detected RTL layout (repo root and hw dir)."""
    hw_dir = _find_rtl_root(_THIS_FILE)
    return RtlLayout(hw_dir=hw_dir, repo_root=hw_dir.parent)


def rtl_dir() -> Path:
    """Convenience accessor for the RTL root directory (`.../1.hw`)."""
    return rtl_layout().hw_dir


def add_sources(platform, rel_files: Iterable[str], base_dir: Path | None = None) -> None:
    """
    Add RTL files to a LiteX platform.

    `rel_files` are paths relative to `base_dir` (default: rtl_dir()).
    Missing files raise FileNotFoundError with a helpful list.
    """
    root_dir = rtl_dir() if base_dir is None else Path(base_dir).resolve()

    missing: list[str] = []
    added:   list[str] = []

    for rel_path in rel_files:
        abs_path = (root_dir / rel_path).resolve()
        if not abs_path.exists():
            missing.append(str(abs_path))
            continue

        platform.add_source(str(abs_path))
        added.append(str(abs_path))

    if missing:
        raise FileNotFoundError(
            "Missing RTL files:\n  " + "\n  ".join(missing) + "\n\n"
            "Hints:\n"
            f"  - base_dir argument = {root_dir}\n"
            f"  - detected rtl_dir() = {rtl_dir()}\n"
            "  - verify: <repo-root>/1.hw/<...>\n"
        )
