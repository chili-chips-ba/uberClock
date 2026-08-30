# docs/source/conf.py
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]  # repo root: 8.python/
SRC_DIR = ROOT / "src"
sys.path.insert(0, str(SRC_DIR))

project = "UberClock SoC"
copyright = "2026, Tarik"
author = "Tarik"
release = "0.1"

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.napoleon",
    "sphinx.ext.viewcode",
]

autosummary_generate = True

autodoc_default_options = {
    "members": True,
    "member-order": "bysource",
    "show-inheritance": True,
}

autodoc_mock_imports = [
    "migen",
    "litex",
    "litex_boards",
    "liteeth",
    "litedram",
]

napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_use_param = True
napoleon_use_rtype = True

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "furo"

html_title = "UberClock SoC"
html_short_title = "UberClock"

html_static_path = ["_static"]
html_css_files = ["custom.css"]

html_theme_options = {
    "light_css_variables": {
        "color-brand-primary": "#2a7ae2",
        "color-brand-content": "#2a7ae2",
    },
    "dark_css_variables": {
        "color-brand-primary": "#7aa7ff",
        "color-brand-content": "#7aa7ff",
    },
}

pygments_style = "default"
pygments_dark_style = "monokai"
