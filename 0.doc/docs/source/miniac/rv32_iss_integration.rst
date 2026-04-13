RV32 ISS Model Integration
==========================

This page summarizes ``4.miniac/4.sim/models/rv32/README.md``.

Overview
--------

This integration layer connects the
`rv32 ISS <https://github.com/wyvernSemi/riscV/tree/main/iss>`_ to the
WireGuard simulation test bench as a program running on VProc. The ISS is
provided as a precompiled static library and the integration headers live in
the local ``include`` directory.

File Organization
-----------------

The ``usercode`` directory contains the main integration sources:

- ``VUserMain0.cpp`` and ``VUserMain.h``
- ``vuserutils.cpp`` and ``vuserutils.h``
- ``rv32_timing_config.h``
- ``rv32_cache.cpp`` and ``rv32_cache.h``
- ``mem_vproc_api.cpp`` and ``mem_vproc_api.h``
- ``uart.cpp`` and ``uart.h``

Main Program Flow
-----------------

``VUserMain0`` is the entry point for VProc node 0 and coordinates ISS setup,
execution, and teardown. The documented flow is:

.. code-block:: text

   Parse vusermain.cfg
   Create an rv32 ISS object
   Run pre-setup for callbacks, timings, and cache
   If GDB mode:
       optionally load an ELF
       service GDB until quit
   Else:
       load the selected ELF
       run until exit
   Run post-run actions
   Clean up
   Sleep to allow simulation to continue

Timing Models
-------------

``rv32_timing_config.h`` provides timing model presets for several supported
cores and variants:

- PICORV32
- EDUBOS5 with 2-stage pipeline
- EDUBOS5 with 3-stage pipeline
- IBEX with single-cycle multiplier
- IBEX with fast multi-cycle multiplier
- IBEX with slow multi-cycle multiplier

User Callback Functions
-----------------------

The integration provides callback hooks for:

- External memory access handling.
- ISS interrupt state updates.
- VProc interrupt forwarding.

The external memory access callback synchronizes simulated time, routes reads
and writes through either the VProc bus path or the direct mem_model API, and
returns wait-state information to the ISS.

Interrupt Mapping
-----------------

The documented interrupt allocation is:

- Bit 0 for the UART interrupt model.
- Bit 2 for the software interrupt.
- Other bits for generic external interrupts.

Utility Functions
-----------------

``vuserutils.cpp`` provides argument parsing and post-run helpers. In the VProc
environment, the configuration is read from ``vusermain.cfg`` rather than from
standard process command-line arguments.

Post-run actions can dump register state, CSR state, and compute an estimated
MIPS rate for bounded instruction runs.

Test Example
------------

The ``riscvtest`` directory contains a small assembly example in ``main.s``
together with a build script ``rv_asm.sh`` and a prebuilt ``main.bin``.
