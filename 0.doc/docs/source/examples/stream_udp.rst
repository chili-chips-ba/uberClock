Stream over UDP
===============

This page documents the host-side convention used by the repository tooling when
streaming captured DDR data over UDP.

Receiver script
---------------

The receiver expects the UBD3 framing:

- magic: ``0x55424433``
- header: ``<IIII`` (magic, seq, offset, total)

Repository helpers
------------------

The build/tooling tree already includes helper scripts such as:

- ``3.build/scripts/receive_udp_capture.py``
- ``3.build/scripts/capture_bin_to_txt.py``
- plotting helpers under ``3.build/scripts``

Typical host workflow:

1. capture to DDR3 on the board,
2. dump or stream chunks from the board/host bridge,
3. reconstruct the binary capture on the host,
4. inspect with plotting scripts.

Example
-------

.. code-block:: bash

   python 3.build/scripts/receive_udp_capture.py

What to validate
----------------

- sequence number continuity,
- byte offset progression,
- total byte count against expected capture size,
- payload shape against ramp-mode expectations before using real ADC data.
