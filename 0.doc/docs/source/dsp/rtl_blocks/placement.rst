.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

Placement Guidelines
====================

The rule for ``1.dsp/rtl`` is intentionally simple:

Included in ``1.dsp/rtl``
-------------------------

- reusable DSP blocks,
- reusable converter-interface blocks,
- generic algorithmic RTL,
- filter, CORDIC, and conversion primitives.

Excluded from ``1.dsp/rtl``
---------------------------

- SoC-specific wrappers,
- CSR and register control logic,
- memory-mapped interfaces,
- board integration and glue logic,
- system-specific capture and routing structures.

Those SoC-specific parts belong in ``2.soc/1.hw`` instead.
