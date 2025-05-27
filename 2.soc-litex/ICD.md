| Offset | Register               | Width (bits) | Fields / Bit-Slices | Description                             |
| :----: | :--------------------- | :----------- | :------------------ | :-------------------------------------- |
|  0x00  | **BYPASS\_EN**         | 1            | \[0]                | 0 = processor path; 1 = raw bypass mode |
|  0x04  | **MUX\_SEL**           | 3            | \[2:0]              | TX channel select (0–4)                 |
|  0x08  | **METHOD\_SEL**        | 3            | \[2:0]              | Operating method (1–5)                  |
|  0x0C  | **UPSAMPLE\_FACTOR**   | 4            | \[3:0]              | Upsample ratio                          |
|  0x10  | **TX\_LPF\_CUTOFF**    | 8            | \[7:0]              | TX low-pass filter cutoff code          |
|  0x14  | **CORDIC\_TX\_PHASE**  | 19           | \[18:0]             | TX-CORDIC phase word                    |
|  0x18  | **DOWNSAMPLE\_FACTOR** | 4            | \[3:0]              | Downsample ratio                        |
|  0x1C  | **RX\_LPF\_CUTOFF**    | 8            | \[7:0]              | RX low-pass filter cutoff code          |
|  0x20  | **CORDIC\_RX\_PHASE**  | 19           | \[18:0]             | RX-CORDIC phase word                    |
|  0x24  | **GAIN\_TX**           | 12           | \[11:0]             | TX gain code                            |
|  0x28  | **GAIN\_RX**           | 12           | \[11:0]             | RX gain code                            |
|  0x2C  | **PHYSICS\_RUN**       | 1            | \[0]                | 1 = start physics engine; 0 = stop      |
|  0x30  | **PHYSICS\_BUSY**      | 1 (status)   | \[0]                | 1 = physics engine busy                 |
|  0x34  | **HS\_DBG\_ADDR**      | 16           | \[15:0]             | High-speed debug RAM address            |
|  0x38  | **HS\_DBG\_WDATA**     | 32           | \[31:0]             | High-speed debug RAM write data         |
|  0x3C  | **HS\_DBG\_RDATA**     | 32 (status)  | \[31:0]             | High-speed debug RAM read data          |
|  0x40  | **LS\_DBG\_ADDR**      | 16           | \[15:0]             | Low-speed debug RAM address             |
|  0x44  | **LS\_DBG\_WDATA**     | 32           | \[31:0]             | Low-speed debug RAM write data          |
|  0x48  | **LS\_DBG\_RDATA**     | 32 (status)  | \[31:0]             | Low-speed debug RAM read data           |
|  0x4C  | **SD\_CMD**            | 8            | \[7:0]              | SD-Card command register                |
|  0x50  | **SD\_STATUS**         | 8 (status)   | \[7:0]              | SD-Card status flags                    |
|  0x54  | **PHASE\_INC**         | 19           | \[18:0]             | CORDIC\_DAC phase increment word        |
