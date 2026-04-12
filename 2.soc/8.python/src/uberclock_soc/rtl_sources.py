from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


def _find_repo_root(start: Path) -> Path:
    """Walk upward from `start` and return the repository root."""
    for parent in [start, *start.parents]:
        if (parent / "1.dsp").is_dir() and (parent / "2.soc").is_dir():
            return parent
    raise RuntimeError(
        "Could not locate repository root.\n"
        f"Search start: {start}\n"
        "Expected layout:\n"
        "  <repo-root>/1.dsp/\n"
        "  <repo-root>/2.soc/\n"
    )


_THIS_FILE = Path(__file__).resolve()


@dataclass(frozen=True)
class RtlLayout:
    """Resolved source layout for this repo."""
    repo_root: Path


def rtl_layout() -> RtlLayout:
    """Return detected source layout rooted at the repository root."""
    repo_root = _find_repo_root(_THIS_FILE)
    return RtlLayout(repo_root=repo_root)


def rtl_dir() -> Path:
    """Compatibility accessor for the repository root used by add_sources()."""
    return rtl_layout().repo_root


def add_sources(platform, rel_files: Iterable[str], base_dir: Path | None = None) -> None:
    """
    Add RTL files to a LiteX platform.

    `rel_files` are paths relative to `base_dir` (default: repository root).
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
            "  - verify paths relative to <repo-root>/\n"
        )
