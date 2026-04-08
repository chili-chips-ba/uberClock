Build the Documentation
=======================

This section explains how to build the uberClock documentation locally and how
it is configured for Read the Docs.

Prerequisites
-------------

Make sure the following tools are installed on your system:

- Python 3
- ``venv`` (Python virtual environments)
- ``make``
- Sphinx

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

   pip install -r 0.doc/docs/requirements.txt

Build the HTML Documentation
----------------------------

Clone the repository if you have not already done so:

.. code-block:: bash

   git clone <repository-url>
   cd <repository-folder>

Build the HTML documentation using Sphinx:

.. code-block:: bash

   cd 0.doc/docs
   make html

After the build finishes, open the generated documentation:

.. code-block:: bash

   xdg-open 0.doc/docs/build/html/index.html

.. note::

   The output HTML files are generated inside the ``build/html`` directory.

Read the Docs
-------------

The repository includes a ``.readthedocs.yaml`` file at the repository root.
Read the Docs uses it to:

- build on Ubuntu 22.04
- use Python 3.11
- load the Sphinx configuration from ``0.doc/docs/source/conf.py``
- install dependencies from ``0.doc/docs/requirements.txt``

After importing the repository into Read the Docs, the hosted documentation
will be available at a URL like:

.. code-block:: text

   https://<project-name>.readthedocs.io/
