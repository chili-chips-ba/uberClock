# Memory RTL

This directory contains the active SoC-side RTL used for the UberDDR3 memory
path and its related data-movement infrastructure.

These files are part of the current LiteX/UberClock integration flow, unlike
the legacy datapath experiments preserved under ``2.soc/1.hw/testing``.

## Main contents

- `ddr3_top.v`
  Top-level DDR3 integration block used by the SoC memory path.

- `ddr3_controller.v`
  Main DDR3 control/state logic.

- `ddr3_phy.v`
  DDR3 PHY-facing logic and timing-sensitive pin/control handling.

- `wbc2pipeline.v`
  Classic Wishbone to pipelined Wishbone bridge used between LiteX-facing bus
  logic and the custom controller path.

- `wbxbar.v`
  Wishbone crossbar used to share the DDR path between CPU access and DMA.

- `zipdma_s2mm.v`
  Stream-to-memory DMA writer used for high-speed capture into DDR.

- `zipdma_rxgears.v`
  DMA/support logic related to receive-side data handling.

- `ub_stream2wb_dma.v`
  Stream-to-Wishbone helper logic for memory transfers.

- `addrdecode.v`
  Address decode helper logic used in the memory subsystem.

- `skidbuffer.v`
  Flow-control / buffering helper used on bus paths.

- `afifo.v`
  Asynchronous FIFO helper.

- `sfifo.v`
  Synchronous FIFO helper.

- `wbxclk.v`
  Wishbone-related clock crossing / support logic.

## Scope

This directory is for active memory/controller RTL that is specific to the
SoC/UberDDR3 integration.

Reusable DSP/math IP belongs in ``1.dsp/rtl``.
Legacy datapath experiments and test tops belong in ``2.soc/1.hw/testing``.
