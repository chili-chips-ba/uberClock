Memory Map
==========

The most important memory distinction in this design is between the standard
LiteX ``main_ram`` path and the optional UberDDR3 side-memory path.

Main RAM
--------

Depending on build configuration, main RAM is either:

- LiteX integrated RAM inside the SoC, or
- external LiteDRAM if integrated RAM is disabled and UberDDR3 is not used.

DDR3 capture area
-----------------

Typical capture base:

- ``0xA0000000`` (example)

In ``soc.py``, UberDDR3 is mapped as a side-memory region:

- base: ``0xA0000000``
- size: ``0x10000000``

Operational implications
------------------------

- firmware link scripts should not accidentally overlap ordinary program memory
  with the capture window,
- host tools should treat this region as capture/storage space rather than
  executable RAM,
- DMA destination addresses should stay within the mapped side-memory aperture.

Make sure your firmware, CSR tooling, and host-side dump/UDP scripts agree on
the DDR window.
