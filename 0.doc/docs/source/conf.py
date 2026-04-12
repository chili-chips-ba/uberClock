from __future__ import annotations

from pathlib import Path
import sys

# -- Project information -----------------------------------------------------

DOCS_DIR = Path(__file__).resolve().parent
REPO_ROOT = DOCS_DIR.parents[2]
PYTHON_SOC_SRC = REPO_ROOT / "2.soc" / "8.python" / "src"

sys.path.insert(0, str(PYTHON_SOC_SRC))

project = 'uberClock'
copyright = '2026, Chili-chips'
author = 'Ahmed Imamovic Tarik Hamedovic'
release = '0.0.1'

# -- General configuration ---------------------------------------------------

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.napoleon",
    "sphinx.ext.viewcode",
    "sphinx_copybutton",
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

templates_path = ['_templates']
exclude_patterns = ['python_soc/api/generated']



# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_logo = '../../artwork/uberclock.logo-no-bg.png'
html_theme_options = {
    'logo_only': True,
}
html_static_path = ['_static']
html_css_files = ['custom.css']
