Capture to DDR3
===============

Workflow
--------

1. Enable capture and configure beat count
2. Trigger DMA request
3. Stream data out using UDP tools

Example commands (firmware console)
-----------------------------------

.. code-block:: text

   cap_enable 1
   cap_beats 4194304
   ub_cap 0xA0000000 4194304

