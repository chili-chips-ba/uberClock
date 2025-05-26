<!---
Markdown description for SystemRDL register map.

Don't override. Generated from: uberclock
  - csr_build/csr.rdl
-->

## uberclock address map

- Absolute Address: 0x0
- Base Offset: 0x0
- Size: 0x2000001C

|  Offset  |Identifier|Name|
|----------|----------|----|
|0x00000000|   imem   |imem|
|0x10000000|   dmem   |dmem|
|0x20000000|    csr   | csr|

## imem memory

- Absolute Address: 0x0
- Base Offset: 0x0
- Size: 0x8000

<p>CPU Program Memory</p>

No supported members.


## dmem memory

- Absolute Address: 0x10000000
- Base Offset: 0x10000000
- Size: 0x8000

<p>CPU Data Memory</p>

No supported members.


## csr address map

- Absolute Address: 0x20000000
- Base Offset: 0x20000000
- Size: 0x1C

<p>uberClock CSR</p>

|Offset|Identifier|     Name     |
|------|----------|--------------|
| 0x00 |   uart   |   csr.uart   |
| 0x10 |   gpio   |   csr.gpio   |
| 0x14 |   hw_id  |   csr.hw_id  |
| 0x18 |hw_version|csr.hw_version|

## uart register file

- Absolute Address: 0x20000000
- Base Offset: 0x0
- Size: 0x10

<p>UART CSR</p>

|Offset|Identifier|        Name       |
|------|----------|-------------------|
|  0x0 |    rx    |    csr.uart.rx    |
|  0x4 |rx_trigger|csr.uart.rx_trigger|
|  0x8 |    tx    |    csr.uart.tx    |
|  0xC |tx_trigger|csr.uart.tx_trigger|

### rx register

- Absolute Address: 0x20000000
- Base Offset: 0x0
- Size: 0x4

<p>UART Rx Register</p>

|Bits|Identifier|Access|Reset|         Name        |
|----|----------|------|-----|---------------------|
| 7:0|   data   |   r  | 0x0 |csr.uart.rx.data[7:0]|
| 30 |   oflow  |   r  | 0x0 |  csr.uart.rx.oflow  |
| 31 |   valid  |   r  | 0x0 |  csr.uart.rx.valid  |

#### data field

<p>Received data</p>

#### oflow field

<p>Indicates that some data bytes are lost</p>

#### valid field

<p>Indicates valid data transfer from the UART to the CPU</p>

### rx_trigger register

- Absolute Address: 0x20000004
- Base Offset: 0x4
- Size: 0x4

<p>UART Rx Trigger Register</p>

|Bits|Identifier|Access|Reset|          Name          |
|----|----------|------|-----|------------------------|
|  0 |   read   |  rw  | 0x0 |csr.uart.rx_trigger.read|

#### read field

<p>Indicates that the CPU can accept next data transfer, used internally - don't try to read or write!</p>

### tx register

- Absolute Address: 0x20000008
- Base Offset: 0x8
- Size: 0x4

<p>UART Tx Register</p>

|Bits|Identifier|Access|Reset|         Name        |
|----|----------|------|-----|---------------------|
| 7:0|   data   |  rw  | 0x0 |csr.uart.tx.data[7:0]|
| 31 |   busy   |   r  | 0x0 |   csr.uart.tx.busy  |

#### data field

<p>Data to send</p>

#### busy field

<p>Indicates that UART cannot accept next data transfer</p>

### tx_trigger register

- Absolute Address: 0x2000000C
- Base Offset: 0xC
- Size: 0x4

<p>UART Tx Trigger Register</p>

|Bits|Identifier|Access|Reset|           Name          |
|----|----------|------|-----|-------------------------|
|  0 |   write  |  rw  | 0x0 |csr.uart.tx_trigger.write|

#### write field

<p>Indicates valid data transfer from the CPU to the UART, used internally - don't try to read or write!</p>

### gpio register

- Absolute Address: 0x20000010
- Base Offset: 0x10
- Size: 0x4

<p>GPIO Register</p>

|Bits|Identifier|Access|Reset|     Name    |
|----|----------|------|-----|-------------|
|  0 |   key1   |   r  | 0x0 |csr.gpio.key1|
|  1 |   key2   |   r  | 0x0 |csr.gpio.key2|
|  8 |   led1   |  rw  | 0x0 |csr.gpio.led1|
|  9 |   led2   |  rw  | 0x0 |csr.gpio.led2|

#### key1 field

<p>Input from KEY1 (0 - not pressed, 1 - pressed)</p>

#### key2 field

<p>Input from KEY2 (0 - not pressed, 1 - pressed)</p>

#### led1 field

<p>Output to LED1 (0 - off, 1 - on)</p>

#### led2 field

<p>Output to LED2 (0 - off, 1 - on)</p>

### hw_id register

- Absolute Address: 0x20000014
- Base Offset: 0x14
- Size: 0x4

<p>Hardware IDs</p>

| Bits|Identifier|Access| Reset|       Name      |
|-----|----------|------|------|-----------------|
| 15:0|  PRODUCT |   r  |0xC10C|csr.hw_id.PRODUCT|
|31:16|  VENDOR  |   r  |0xCCAE| csr.hw_id.VENDOR|

#### PRODUCT field

<p>Product ID</p>

#### VENDOR field

<p>Vendor ID</p>

### hw_version register

- Absolute Address: 0x20000018
- Base Offset: 0x18
- Size: 0x4

<p>Hardware Version</p>

| Bits|Identifier|Access|Reset|        Name        |
|-----|----------|------|-----|--------------------|
| 15:0|   PATCH  |   r  | 0x0 |csr.hw_version.PATCH|
|23:16|   MINOR  |   r  | 0x1 |csr.hw_version.MINOR|
|31:24|   MAJOR  |   r  | 0x0 |csr.hw_version.MAJOR|

#### PATCH field

<p>Patch version</p>

#### MINOR field

<p>Minor version</p>

#### MAJOR field

<p>Major version</p>
