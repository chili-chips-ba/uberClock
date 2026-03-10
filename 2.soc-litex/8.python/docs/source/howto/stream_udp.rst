Stream over UDP
===============

Receiver script
---------------

The receiver expects the UBD3 framing:

- magic: ``0x55424433``
- header: ``<IIII`` (magic, seq, offset, total)

Example
-------

.. code-block:: bash

   python recv_udp.py --port 5000 --out capture.bin

