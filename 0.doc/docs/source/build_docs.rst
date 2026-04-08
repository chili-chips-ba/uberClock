Build the Documentation
=======================

This section explains how to build the DV Power OS documentation locally.

Prerequisites
-------------

Make sure the following tools are installed on your system:

- Python 3
- ``venv`` (Python virtual environments)
- Sphinx and required extensions:
  - ``sphinx``
  - ``furo`` (theme)
  - ``sphinx-copybutton``
  - ``sphinx-rtd-theme`` (optional)

Setup a Virtual Environment
---------------------------

Create a virtual environment for building the documentation:

.. code-block:: bash

   python3 -m venv ~/venvs/docs

Activate the environment:

.. code-block:: bash

   source ~/venvs/docs/bin/activate

Install Documentation Dependencies
----------------------------------

Install the required Python packages:

.. code-block:: bash

   pip install -U sphinx sphinx-rtd-theme furo sphinx-copybutton

Build the HTML Documentation
----------------------------

Clone the repository if you have not already done so:

.. code-block:: bash

   git clone <repository-url>
   cd <repository-folder>

Build the HTML documentation using Sphinx:

.. code-block:: bash

   make html

After the build finishes, open the generated documentation:

.. code-block:: bash

   xdg-open build/html/index.html

.. note::

   The output HTML files are generated inside the ``build/html`` directory.
