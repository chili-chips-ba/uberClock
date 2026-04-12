Quickstart
==========

This quickstart assumes you are at the repository root and already have a
working Python environment for LiteX development.

Install the Python package
--------------------------

Editable install keeps the package importable while you edit the source tree.

.. code-block:: bash

   python -m pip install -e 2.soc/8.python

If you use the build Makefile under ``3.build``, you can also run:

.. code-block:: bash

   make -C 3.build install-python

Build a bitstream
-----------------

The preferred package entry point is:

.. code-block:: bash

   python -m uberclock_soc --build --with-uberclock --with-uberddr3 --with-ethernet

Equivalent workflow through the repository Makefile:

.. code-block:: bash

   make -C 3.build build-board \
     OPTIONS="--with-uberclock --with-uberddr3 --with-ethernet"

Generate the docs
-----------------

The canonical published docs site is the main Sphinx project under
``0.doc/docs``.

.. code-block:: bash

   python -m pip install -r 0.doc/docs/requirements.txt
   python -m sphinx -b html 0.doc/docs/source 0.doc/docs/build/html

Open:

.. code-block:: text

   0.doc/docs/build/html/index.html

Run local software/demo flow
----------------------------

After building the SoC, the software/demo tree can be driven with:

.. code-block:: bash

   make -C 3.build build-sw
   make -C 3.build term PORT=/dev/ttyUSB0

Sanity checks
-------------

- confirm the Python entry point imports: ``python -m uberclock_soc --help``
- confirm the docs build includes the ``SoC`` section in the main index
- confirm the generated CSR CSV is present under ``3.build/build/<board>/``
