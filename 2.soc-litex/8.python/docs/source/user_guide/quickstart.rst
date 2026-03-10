Quickstart
==========

Build the docs
--------------

From the repository root (``8.python/``):

.. code-block:: bash

   pip install furo
   rm -rf docs/build
   sphinx-build -b html docs/source docs/build/html
   xdg-open docs/build/html/index.html

