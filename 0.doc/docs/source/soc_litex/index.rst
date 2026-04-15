SoC Platform
============

This section documents the LiteX-based SoC used in uberClock: what lives in the
SoC tree, how the hardware and software layers fit together, and where the
Python and firmware control surfaces sit.

If you are new to the SoC tree, read the pages in this order:

1. ``overview`` for the repository layout and the CPU-controlled system role.
2. ``python_soc/index`` for the Python integration and API surface.
3. ``testing`` for the preserved experimental tops.
4. the ``Examples`` and ``Build`` sections for actual workflows.

.. toctree::
   :maxdepth: 1
   :caption: Structure

   overview
   Python Integration and API <../python_soc/index>
   Testing and Experiments <testing>

.. toctree::
   :maxdepth: 1
   :caption: Reference

   ../python_soc/reference/csr_map
   ../python_soc/reference/memory_map
