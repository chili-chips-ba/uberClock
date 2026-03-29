Build the SoC
=============

Typical LiteX build invocation:

.. code-block:: bash

   ./your_build_script.py --sys-clk-freq 65e6

Notes:

- Ensure the generated CSR header matches your firmware.
- Keep the clock constraints consistent with the DDR3 PHY clocks.
- The default SoC build keeps ``main_ram`` in 512 KiB of integrated BRAM.
- To use external DDR3 as CPU/firmware memory, build with ``--with-sdram-main-ram``.
- ``--with-sdram-main-ram`` and ``--with-uberddr3`` are mutually exclusive because both consume the same DDR3 device.

For the bare-metal demo app, large static buffers and the heap come from ``.bss``/``.data``, which are
controlled separately by ``2.sw/demo.py``. To place both code and data in DDR-backed ``main_ram``:

.. code-block:: bash

   python 2.sw/demo.py --build-path <build-dir> --mem main_ram --data-mem main_ram
