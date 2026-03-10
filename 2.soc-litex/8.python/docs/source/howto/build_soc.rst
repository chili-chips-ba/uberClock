Build the SoC
=============

Typical LiteX build invocation:

.. code-block:: bash

   ./your_build_script.py --sys-clk-freq 65e6

Notes:

- Ensure the generated CSR header matches your firmware.
- Keep the clock constraints consistent with the DDR3 PHY clocks.

