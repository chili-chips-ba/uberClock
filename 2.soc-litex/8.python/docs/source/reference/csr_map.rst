CSR Map
=======

This section documents key CSRs exposed by the SoC (DMA, capture controls, status).

DMA control
-----------

- ``dma_req``: start a DMA write (strobe)
- ``dma_addr0/1``: base address
- ``dma_inc``: increment address enable
- ``dma_size``: transfer size encoding

DMA status
----------

- ``dma_busy``: DMA in progress
- ``dma_err``: error sticky flag

