Python Integration and API
==========================

.. note::

   This page collects the Python-facing part of the SoC: the packaged LiteX
   integration modules under ``src/uberclock_soc`` and the generated API
   documentation for them.

The Python package exists to do three jobs cleanly:

- assemble the AX7203 SoC without keeping all LiteX integration in one target file,
- wrap custom RTL blocks such as UberClock and UberDDR3 in reusable Python modules,
- document the software-visible control surface, capture flow, and module API.

If you are new to this tree, start from the parent ``SoC Platform`` section
first. This page focuses on the Python implementation layer and the generated
API reference.

.. toctree::
   :maxdepth: 2
   :caption: Package Structure

   user_guide/overview
   user_guide/architecture

.. toctree::
   :maxdepth: 2
   :caption: Python API

   api/index
