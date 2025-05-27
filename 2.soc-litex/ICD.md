**MAIN ICD Register Bank**  
All of the registers listed below live in the MAIN ICD bank, which is memory-mapped at `CSR_MAIN_BASE = CSR_BASE + 0x2000` (i.e. address `0xF000_2000`).
Each entry occupies one full 32-bit word in the CSR bus.  Only the bits shown under “Bits” are implemented; all other bit-positions are reserved and should be written as zero (reads return zero).



| Offset | Bits     | Field           | Access | Description                             |
|:------:|:--------:|:----------------|:-------|:----------------------------------------|
| 0x00   | [0]      | BYPASS_EN       | RW     | 0 = processor path; 1 = raw bypass mode |
|        | [31:1]   | Reserved        | —      | —                                       |
| 0x04   | [2:0]    | MUX_SEL         | RW     | TX channel select (0–4)                 |
|        | [31:3]   | Reserved        | —      | —                                       |
| 0x08   | [2:0]    | METHOD_SEL      | RW     | Operating method (1–5)                  |
|        | [31:3]   | Reserved        | —      | —                                       |
| 0x0C   | [3:0]    | UPSAMPLE_FACTOR | RW     | Upsample ratio                          |
|        | [31:4]   | Reserved        | —      | —                                       |
| 0x10   | [7:0]    | TX_LPF_CUTOFF   | RW     | TX low-pass filter cutoff code          |
|        | [31:8]   | Reserved        | —      | —                                       |
| 0x14   | [18:0]   | CORDIC_TX_PHASE | RW     | TX-CORDIC phase word                    |
|        | [31:19]  | Reserved        | —      | —                                       |
| 0x18   | [3:0]    | DOWNSAMPLE_FACTOR| RW    | Downsample ratio                        |
|        | [31:4]   | Reserved        | —      | —                                       |
| 0x1C   | [7:0]    | RX_LPF_CUTOFF   | RW     | RX low-pass filter cutoff code          |
|        | [31:8]   | Reserved        | —      | —                                       |
| 0x20   | [18:0]   | CORDIC_RX_PHASE | RW     | RX-CORDIC phase word                    |
|        | [31:19]  | Reserved        | —      | —                                       |
| 0x24   | [11:0]   | GAIN_TX         | RW     | TX gain code                            |
|        | [31:12]  | Reserved        | —      | —                                       |
| 0x28   | [11:0]   | GAIN_RX         | RW     | RX gain code                            |
|        | [31:12]  | Reserved        | —      | —                                       |
| 0x2C   | [0]      | PHYSICS_RUN     | RW     | 1 = start physics engine; 0 = stop      |
|        | [31:1]   | Reserved        | —      | —                                       |
| 0x30   | [0]      | PHYSICS_BUSY    | R      | 1 = physics engine busy                 |
|        | [31:1]   | Reserved        | —      | —                                       |
| 0x34   | [15:0]   | HS_DBG_ADDR     | RW     | High-speed debug RAM address            |
|        | [31:16]  | Reserved        | —      | —                                       |
| 0x38   | [31:0]   | HS_DBG_WDATA    | RW     | High-speed debug RAM write data         |
| 0x3C   | [31:0]   | HS_DBG_RDATA    | R      | High-speed debug RAM read data          |
| 0x40   | [15:0]   | LS_DBG_ADDR     | RW     | Low-speed debug RAM address             |
|        | [31:16]  | Reserved        | —      | —                                       |
| 0x44   | [31:0]   | LS_DBG_WDATA    | RW     | Low-speed debug RAM write data          |
| 0x48   | [31:0]   | LS_DBG_RDATA    | R      | Low-speed debug RAM read data           |
| 0x4C   | [7:0]    | SD_CMD          | RW     | SD-Card command register                |
|        | [31:8]   | Reserved        | —      | —                                       |
| 0x50   | [7:0]    | SD_STATUS       | R      | SD-Card status flags                    |
|        | [31:8]   | Reserved        | —      | —                                       |
| 0x54   | [18:0]   | PHASE_INC       | RW     | CORDIC_DAC phase-increment word         |
|        | [31:19]  | Reserved        | —      | —                                       |
