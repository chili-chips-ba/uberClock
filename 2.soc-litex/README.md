# LiteX-based SOC Architecture

## High Speed Memory Debug Architecture

<img width="1766" height="885" alt="debug drawio" src="https://github.com/user-attachments/assets/04a752bc-5896-459f-857c-83a84d601659" />


## High Speed Debug Captures

<img width="1500" height="600" alt="try1" src="https://github.com/user-attachments/assets/910a435e-bfb2-4fce-8741-27b30802a760" />

## Parameters

### Clocking

| Parameter                     | Value          | Notes                                       |
|------------------------------|----------------|---------------------------------------------|
| Input board clock            | 200 MHz        | Differential `clk200_p/n`                   |
| System clock domain          | 100 MHz        | `cd_sys` (CPU, CSR, DDR controller side)    |
| UberClock clock domain       | 65 MHz         | `cd_uc` (UberClock DSP core)                |
| DDR3 PHY clock               | 400 MHz        | `cd_ub_4x` (DDR internal)                   |
| DDR3 DQS clock               | 400 MHz (+90°) | `cd_ub_4x_dqs` (DQS, phase shifted by 90°)  |
| IDELAY control domain        | 200 MHz        | `cd_idelay` via BUFG                        |
| MMCM speedgrade              | -2             | For S7MMCM PLLs                             |
| `create_clkout` margin       | 1e-2           | LiteX MMCM margin parameter                 |

---

### UberDDR3

| Parameter            | Value     | Notes                                                |
|----------------------|-----------|------------------------------------------------------|
| `sys_clk_hz`         | 100e6     | System/controller clock frequency                    |
| `ddr_ck_hz`          | 400e6     | DDR3 CK frequency                                   |
| `ctrl_ps`            | 10000 ps  | `round(1e12 / 100e6)` – controller clock period      |
| `ddr_ps`             | 2500 ps   | `round(1e12 / 400e6)` – DDR clock period             |
| `row_bits`           | 15        | Row address bits                                    |
| `col_bits`           | 10        | Column address bits                                 |
| `ba_bits`            | 3         | Bank address bits                                   |
| `byte_lanes`         | 4         | 4 lanes × 16-bit = 64-bit external DDR data bus     |
| `dual_rank`          | 0         | Single-rank DIMM                                     |
| `speed_bin`          | 3         | DDR3 speed-bin index                                 |
| `sdram_capacity`     | 5         | Design-specific SDRAM size encoding                  |
| `dll_off`            | 0         | DLL enabled                                          |
| `odelay_supported`   | 0         | No ODELAY support (in this config)                  |
| `bist_mode`          | 0         | BIST disabled                                        |
| `p_DIC`              | 0b01      | Output drive impedance setting                       |
| `p_RTT_NOM`          | 0b001     | Nominal on-die termination                           |
| `p_AUX_WIDTH`        | 4         | Auxiliary bus width                                  |
| INTERNAL_VREF        | 0.75 V    | DDR IO bank internal reference voltage              |
| `ub_dw`              | 256 bits  | Internal wide data bus (`64 × byte_lanes`)          |
| `serdes_ratio`       | 4         | SERDES ratio inside controller                       |
| `wb_addr_bits`       | 25        | `15 + 10 + 3 − log₂(8) + 0 = 25`                     |

---

### Bus widths & addresses

| Parameter                  | Value              | Notes                                                       |
|----------------------------|--------------------|-------------------------------------------------------------|
| CPU-side WB width          | 32 bits            | `self.wb`: classic Wishbone slave on LiteX bus             |
| Wide WB width              | 256 bits           | `wb_wide.data_width = ub_dw`                               |
| Wide WB address width      | SoC-dependent      | `wb_wide.adr_width` from LiteX                             |
| Pipelined WB address width | `AW`               | `AW = len(self.c2p.m_adr)`                                 |
| Pipelined WB data width    | `DW = 256 bits`    | Same as `ub_dw`                                            |
| Crossbar masters (`NM`)    | 2                  | Master 0 = CPU, Master 1 = DMA                             |
| Crossbar slaves (`NS`)     | 1                  | Slave 0 = DDR3 controller                                  |
| `p_LGMAXBURST`             | 6                  | Maximum burst = 2⁶ beats                                   |
| `p_OPT_TIMEOUT`            | 0                  | Timeout disabled                                           |
| `p_OPT_STARVATION_TIMEOUT` | 0                  | Starvation timeout disabled                                |
| `p_OPT_DBLBUFFER`          | 0                  | Double-buffering disabled                                  |
| `p_OPT_LOWPOWER`           | 1                  | Low-power mode enabled                                     |
| `WBLSB`                    | 5                  | `log₂(DW/8) = log₂(256/8) = log₂(32) = 5`                  |
| `ADDR_WIDTH_FOR_DMA`       | `AW + 5`           | Byte address width for DMA (counts bytes, not bus words)   |

---

### 2.4 Capture / stream parameters

| Block             | Parameter      | Value         | Notes                                                  |
|-------------------|----------------|---------------|--------------------------------------------------------|
| **RampSource**    | `dw`           | 256 bits      | Stream data width                                      |
|                   | `length`       | 256 beats     | Number of beats per capture                           |
|                   | `lanes`        | 16            | `dw / 16` → 16 lanes of 16-bit values per beat         |
|                   | Samples/run    | 4096          | `256 beats × 16 lanes` (16-bit test ramp)             |
| **SampleStream**  | `sample_width` | 12 bits       | Captured input (`cap_sample`)                         |
|                   | `dw`           | 256 bits      | Stream data width                                      |
|                   | `beats`        | 256           | Beats per run                                         |
|                   | `lanes`        | 16            | 16 samples per beat                                   |
|                   | Samples/run    | 4096          | `256 × 16` 12-bit samples                             |
| **UCStreamSource**| `dw`           | 256 bits      | Same as above                                         |
|                   | `length`       | 256 beats     | Matches Ramp/SampleStream length                      |
|                   | `use_external` | 0/1           | 0 = ramp, 1 = external `SampleStream`                 |
| **AsyncFIFO (uc→sys)** | `bytes_width` | `len(s_bytes)` | Enough to encode `DW/8` bytes                    |
|                   | `fifo_width`   | `DW + bytes_width + 1` | Data + bytes + last flag                    |
|                   | `depth`        | 4             | UC-to-SYS stream FIFO depth                           |
| **zipdma_s2mm**   | `p_ADDRESS_WIDTH` | `ADDR_WIDTH_FOR_DMA` | DMA byte address width                     |
|                   | `p_BUS_WIDTH`  | 256 bits      | Bus width for writes to DDR                           |
|                   | `p_OPT_LITTLE_ENDIAN` | 1      | Little-endian                                         |
|                   | `p_LGPIPE`     | 10            | Internal pipeline depth configuration                 |

---

### 2.5 CSR / configuration parameters

#### MainCSRs

| Field                  | Width | Notes                                        |
|------------------------|-------|----------------------------------------------|
| `phase_inc_nco`        | 19    | NCO phase increment                          |
| `phase_inc_down_1..5`  | 19    | Phase increments for downsamplers 1–5       |
| `phase_inc_cpu`        | 19    | CPU-side phase increment                     |
| `input_select`         | 2     | Input routing select                         |
| `output_select_ch1`    | 2     | Output routing for channel 1                 |
| `output_select_ch2`    | 2     | Output routing for channel 2                 |
| `upsampler_input_mux`  | 2     | Upsampler input selection                    |
| `gain1..5`             | 32    | 32-bit fixed-point gains                     |
| `upsampler_input_x`    | 16    | Direct upsampler input (I)                   |
| `upsampler_input_y`    | 16    | Direct upsampler input (Q)                   |
| `final_shift`          | 3     | Output scaling/shift                         |
| `cap_enable`           | 1     | `1 = capture design`, `0 = internal ramp`    |

#### CSRConfigAFIFO

| Parameter     | Value | Notes                                                     |
|---------------|-------|-----------------------------------------------------------|
| `fifo_depth`  | 4     | Config frames buffered (sys→uc)                           |
| `total_w`     | –     | Sum of all config field widths (packed frame)            |
| `level` width | 8     | Status CSR; bit0 = readable, bit1 = writable             |

#### DMA CSRs

| CSR         | Width | Notes                                                  |
|-------------|-------|--------------------------------------------------------|
| `dma_req`   | 1     | Edge-triggered start request                           |
| `dma_busy`  | 1     | DMA busy status                                        |
| `dma_err`   | 1     | DMA error status                                       |
| `dma_inc`   | 1     | Address increment enable (default = 1)                |
| `dma_size`  | 2     | `00=bus, 01=32b, 10=16b, 11=byte`                      |
| `dma_addr0` | 32    | Lower 32 bits of DMA address                           |
| `dma_addr1` | 32    | Upper 32 bits of DMA address                           |

---

### 2.6 Memory map / SoC

| Item                     | Value          | Notes                                        |
|--------------------------|----------------|----------------------------------------------|
| UberDDR3 base address    | `0xA000_0000`  | `ub_base` – start of side DDR3 region        |
| UberDDR3 size            | `0x1000_0000`  | 256 MiB (`ub_size`)                          |
| UberDDR3 region flags    | `cached=False` | `linker=False`                               |
| Firmware constant        | `UBDDR3_MEM_BASE` | Exported to C firmware                    |
| Integrated main RAM size | 64 KiB         | Default `integrated_main_ram_size`           |
| SoC ident string         | `"AX7203 UberClock65 UberDDR3 with S2MM via wbxbar"` | Printed at boot |


#### End-of-Document
