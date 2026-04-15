Memory Map
==========

This page summarizes the generated memory map of the uberClock LiteX SoC.

The memory map defines where firmware code, RAM, peripherals, DDR-backed memory,
and the CSR window appear in the CPU address space.

Overview
--------

The generated LiteX memory regions are:

.. code-block:: text

   ROM        0x00000000  0x00020000
   SRAM       0x10000000  0x00002000
   MAIN_RAM   0x40000000  0x00040000
   UB_RAM     0xA0000000  0x10000000
   ETHMAC     0x80000000  0x00002000
   ETHMAC_RX  0x80000000  0x00001000
   ETHMAC_TX  0x80001000  0x00001000
   CSR        0xF0000000  0x00010000

Firmware placement
------------------

The linker script places sections as follows:

- ``.text`` in ``main_ram``
- ``.rodata`` in ``main_ram``
- ``.data`` in ``data_ram`` with load image in ``main_ram``
- ``.bss`` in ``data_ram``

In the current configuration, ``main_ram`` is the main executable/data memory
used by firmware.

Main regions
------------

ROM
~~~

Base:

.. code-block:: text

   0x00000000

Size:

.. code-block:: text

   0x00020000

This is the SoC ROM region.

SRAM
~~~~

Base:

.. code-block:: text

   0x10000000

Size:

.. code-block:: text

   0x00002000

This is a small on-chip SRAM region.

MAIN_RAM
~~~~~~~~

Base:

.. code-block:: text

   0x40000000

Size:

.. code-block:: text

   0x00040000

This is the main firmware execution/data memory region and is the most important
RAM region for the software build.

UB_RAM
~~~~~~

Base:

.. code-block:: text

   0xA0000000

Size:

.. code-block:: text

   0x10000000

This is the UberDDR3 side-memory window used for high-speed capture and bulk
data movement.

The DDR3-backed capture path writes data here via DMA, and host-side export
tools read from this region indirectly through the SoC transport path.

ETHMAC / ETHMAC_RX / ETHMAC_TX
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Base regions:

.. code-block:: text

   ETHMAC     0x80000000  0x00002000
   ETHMAC_RX  0x80000000  0x00001000
   ETHMAC_TX  0x80001000  0x00001000

These regions belong to the LiteEth MAC buffers and data path.

They are part of the transport infrastructure used when moving data over the
Ethernet interface.

CSR window
~~~~~~~~~~

Base:

.. code-block:: text

   0xF0000000

Size:

.. code-block:: text

   0x00010000

This region contains all LiteX-generated control and status registers. The
firmware uses this window to control the DSP datapath, FIFOs, capture logic,
DMA engine, UART, and other peripherals.

Linker view
-----------

The linker script uses symbolic regions such as:

- ``main_ram``
- ``data_ram``

and places the software sections into them.

Conceptually:

.. code-block:: text

   .text    → main_ram
   .rodata  → main_ram
   .data    → data_ram   (load image in main_ram)
   .bss     → data_ram

The stack is placed at the top of ``data_ram``, and the heap starts after
``.bss``.

Low-speed capture
~~~~~~~~~~~~~~~~~

Low-speed capture uses CSR-visible registers and does not require large external
memory windows.

High-speed capture
~~~~~~~~~~~~~~~~~~

High-speed capture writes into ``UB_RAM`` using the ``UBDDR3`` DMA control path.
That is why the DDR3 region is a central part of the runtime debug and analysis
workflow.

See also
--------

- :doc:`csr_map`
