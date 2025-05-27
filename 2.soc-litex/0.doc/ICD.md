**MAIN ICD Register Bank**  
All of the registers listed below live in the MAIN ICD bank, which is memory-mapped at `CSR_MAIN_BASE = CSR_BASE + 0x2000` (i.e. address `0xF000_2000`).
Each entry occupies one full 32-bit word in the CSR bus.  Only the bits shown under “Bits” are implemented; all other bit-positions are reserved and should be written as zero (reads return zero).

TODO:

Specify the  5 algorithms.
RAM Bandwith? Is the CPU going to be fighting with the HS Capture?
Picture for Gain.
HS CAPTURE: What control is starting? Am I always capturing? Where am I going to stop? What controls it?

| Offset |   Bits   | Field                  | Access | Description                                              |
| :----: | :------: | :--------------------- | :----- | :------------------------------------------------------- |
|  0x00  |   \[0]   | **BYPASS\_EN**         | RW     | 0 = CPU processing chain; 1 = raw bypass (pass-through)  |
|        |  \[31:1] | Reserved               | —      | —                                                        |
|  0x04  |  \[2:0]  | **MUX\_SEL**           | RW     | Select which of the 5 frequency paths to output (0…4)    |
|        |  \[31:3] | Reserved               | —      | —                                                        |
|  0x08  |  \[2:0]  | **METHOD\_SEL**        | RW     | Select algorithm variant (1…5)                           |
|        |  \[31:4] | Reserved               | —      | —                                                        |
|  0x14  |  \[11:0] | **GAIN0**              | RW     | Gain setting for path 0                                  |
|        | \[31:12] | Reserved               | —      | —                                                        |
|  0x18  |  \[11:0] | **GAIN1**              | RW     | Gain setting for path 1                                  |
|        | \[31:12] | Reserved               | —      | —                                                        |
|  0x1C  |  \[11:0] | **GAIN2**              | RW     | Gain setting for path 2                                  |
|        | \[31:12] | Reserved               | —      | —                                                        |
|  0x20  |  \[11:0] | **GAIN3**              | RW     | Gain setting for path 3                                  |
|        | \[31:12] | Reserved               | —      | —                                                        |
|  0x24  |  \[11:0] | **GAIN4**              | RW     | Gain setting for path 4                                  |
|        | \[31:12] | Reserved               | —      | —                                                        |
|  0x28  |  \[15:0] | **HS\_DBG\_ADDR**      | RW     | High-speed debug RAM address                             |
|        | \[31:16] | Reserved               | —      | —                                                        |
|  0x2C  |  \[31:0] | **HS\_DBG\_WDATA**     | RW     | High-speed debug RAM write data                          |
|  0x30  |  \[31:0] | **HS\_DBG\_RDATA**     | R      | High-speed debug RAM read data                           |
|  0x34  |  \[15:0] | **LS\_DBG\_ADDR**      | RW     | Low-speed debug RAM address                              |
|        | \[31:16] | Reserved               | —      | —                                                        |
|  0x38  |  \[31:0] | **LS\_DBG\_WDATA**     | RW     | Low-speed debug RAM write data                           |
|  0x3C  |  \[31:0] | **LS\_DBG\_RDATA**     | R      | Low-speed debug RAM read data                            |
