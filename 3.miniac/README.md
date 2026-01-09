# Miniac

## Introduction

The Miniac presents a comprehensive study and implementation of high-speed Analog-to-Digital Converter (ADC) data acquisition using a Field-Programmable Gate Array (FPGA) and a Personal Computer (PC). The core challenge addressed is achieving sufficiently high sampling frequencies for real-time signal analysis, particularly when constrained by common serial communication protocols like UART.

We explore two distinct methodologies: a Direct Method that involves real-time, sample-by-sample data transfer, and an Indirect (Batch Processing) Method designed to overcome the inherent limitations of the direct approach. This project demonstrates how FPGA-side buffering and intelligent data transfer strategies can dramatically improve effective sampling rates, making the acquisition of higher-frequency signals feasible for various applications. This README details the hardware and software architecture, the implementation of both methods, their comparative performance, and provides instructions for replication.

- Overview of the hardware setup
<p align="center">
    <img width=600 src="0.doc/setUp.jpg">
</p>

## Project overview

### Hardware components

- [AX7203 Artix7-200 FPGA Board](https://www.en.alinx.com/Product/FPGA-Development-Boards/Artix-7/AX7203.html)
<p align="center">
  <img width=600 src="0.doc/FPGA-Board--Artix7-200--AX7203.jpg">
</p>
  
- [AN9238 2xADC,  65MSPS, 12bit](https://www.en.alinx.com/Product/Add-on-Modules/AN9238.html)
<p align="center">
    <img width=300 src="0.doc/2xADC--65MSPS-12bit--AN9238.jpg">
</p>


## Methodology

The Miniac is a tool for data acquisition and visualizationation. As can be seen on teh diagram below, first we use our ADC to sample sine signals from a signal generator, after which we send our data to the Control and Status Registers (CSR). From here we have two options, one is to take the data directly from the CSR and send it via UART to our PC, where we will then run our Python code for visualization and dsp, or we can collect the data from CSR with our RISC-V processor and store the data in DMEM. Once we store this data we can then send all of it (once again via UART) to our PC. There we will have another Python code ready to read the data and do visualization and FFT. The two options we described are our Direct and Indirect snooping methods, respectfully.

<p align="center">
  <img width=900 src="0.doc/miniac.png">
</p> 

In this section we will present the three implemented methods: 

### 1. Direct snooping method

This method uses the Python file "PlotDataADC.py" (https://github.com/chili-chips-ba/uberClock/blob/main/3.miniac/5.test/PlotDataADC.py). This Python script acts as a real-time signal monitoring tool, communicating with an FPGA over UART to continuously acquire ADC samples. It dynamically measures the actual sampling frequency and provides live visualizations, displaying both the reconstructed signal in the time domain and its frequency spectrum via FFT analysis, including automatic peak detection. The script demonstrates the direct, sample-by-sample data acquisition method. We expected the sampling frequency of this method to be about 1000[Hz], but the experiment showed that we could only sample and recreate a sine of about 100
[Hz]. We demonstrate the functionality of our method on the pictures below.

- Sampling a 50[Hz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/PlotDataADC_50Hz.png">
</p>

- Sampling a 100[Hz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/PlotDataADC_100Hz.png">
</p>

- Sampling a 120[Hz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/PlotDataADC_120Hz.png">
</p>

---
### 2. Indirect snooping method

For this method we run "CPUSnooping.py" (https://github.com/chili-chips-ba/uberClock/blob/main/3.miniac/5.test/CPUSnooping.py). This Python script implements the Indirect (Batch Processing) Method for ADC data acquisition. It operates by periodically reading a complete buffer of 1024 pre-sampled 32-bit values from a specific memory address on the FPGA board via UART. After receiving and processing the entire data block, which includes extracting the 12-bit ADC values, performing signal reconstruction, and analyzing the frequency spectrum, the script pauses its communication. This pause allows the FPGA's internal CPU to autonomously refill the circular buffer at a high speed, before the PC re-establishes communication to fetch the next batch of data for continued visualization and analysis.

The fundamental difference between this script and the direct method lies in their data transfer strategy and overall efficiency. The first script continuously polls the FPGA for individual ADC samples, leading to significant UART overhead and limiting the achievable sampling frequency to approximately 100 Hz due to the overhead of numerous small transactions and PC-side latencies. In contrast, this indirect method batches data transfer by reading large blocks (1024 samples) at once, and crucially, allows the FPGA to sample and buffer data autonomously without constant PC intervention. This approach drastically reduces the number of UART transactions and minimizes PC-side communication overhead, enabling the system to achieve a much higher effective sampling frequency of up to 250 [kHz], making it far more suitable for acquiring higher-frequency signals. We demonstrate the functionality of our method on the pictures below.

- Sampling a 50[Hz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/CPUSnooping_100kHz.png">
</p>

- Sampling a 200[kHz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/CPUSnooping_200kHz.png">
</p>

- Sampling a 250[kHz] sine with the direct method
<p align="center">
    <img width=600 src="0.doc/CPUSnooping_250kHz.png">
</p>

---

### 3. DMA-Based ADC Snapshot Method (Hardware Acceleration)

The pinnacle of the Miniac's evolution is the transition from **Processor-In-the-Loop (PIO)** acquisition to a hardware-accelerated **DMA (Direct Memory Access)** approach using **Dual-Port BRAM**. This method shatters the previous 250 kHz limitation, enabling full-speed acquisition at the ADC's native sampling rate of **65 MSPS**.



### Architecture and Data Flow
In the previous "Indirect Method," the RISC-V CPU was responsible for reading samples from the ADC CSR and writing them to memory. This created a bottleneck due to CPU instruction overhead.

The new DMA ADC Snapshot architecture introduces a dedicated `adc_mem_controller`:

* **Dual-Port RAM Isolation:** Port A is connected to the RISC-V CPU, while Port B is dedicated to the ADC Controller.
* **Hardware Triggering:** When the CPU sets an enable bit, the hardware controller takes absolute control, streaming 12-bit samples at a constant **65 MHz** clock.
* **Zero CPU Overhead:** The CPU is free to perform other tasks while the hardware fills the "snapshot" buffer.
* **Post-Processing:** Once full, the CPU reads data via Port A and transmits it via UART. Since the data is already captured, UART speed no longer affects signal fidelity.

### System Block Diagram
<p align="center">
  <img width=600 src="0.doc/DMA_ADC_DPRAM.png">
  <br><em>DMA-Based Architecture with Dual-Port BRAM</em>
</p>
The DMA-based architecture consists of three core components:

* **ADC Controller:** Dedicated hardware logic that awaits a "trigger" signal from the CPU and then directly streams 12-bit data from the ADC into memory without processor intervention.
* **Dual-Port BRAM:** A high-speed memory buffer acting as a bridge between clock domains. **Port B** is dedicated exclusively to the ADC (Write-only), while **Port A** is reserved for the CPU (Read-only).
* **Control & Status Registers (CSR):** The command interface through which the CPU initiates acquisition (via the *start* bit) and monitors the status to check if the buffer is full (via the *done* bit).

---

### Simulation & Verification

This section documents the functional verification of the ADC Memory Controller within the FPGA SoC environment. The simulation was performed at a system clock frequency of **65 MHz**, matching the operational frequency of the CPU and ADC logic.

### Functional Simulation Results

The following analysis is based on the behavioral simulation of the `top_tb` module, which integrates the ADC Controller, CSR registers, and Dual-Port BRAM.

```mermaid 
flowchart TD
    ST(( )) --> IDLE

    IDLE(["<b>IDLE</b><br/>adc_we_o = 0<br/>csr_done_o = 0<br/>addr = 0x0400"])
    RUNNING(["<b>RUNNING</b><br/>adc_we_o = 1<br/>csr_done_o = 0<br/>addr++"])
    DONE(["<b>DONE</b><br/>adc_we_o = 0<br/>csr_done_o = 1"])

    IDLE -- "csr_start_i == 1" --> RUNNING
    RUNNING -- "addr == 0x13FF" --> DONE
    DONE -- "csr_start_i == 0" --> IDLE
```

### 1. Acquisition Initiation (IDLE -> RUNNING)
The simulation confirms the FSM successfully transitions from the **IDLE** state to **RUNNING** upon receiving a trigger pulse.
* **Trigger Mechanism**: The `csr_start_i` pulse initiates the capture sequence.
* **Write Enable**: The `adc_we_o` signal is asserted synchronously with the first valid sample, enabling data storage into the Port B of the DPRAM.
* **Addressing**: The address counter begins at the base address `0x0400`. 

### 2. Data Integrity and Pipeline Latency
The system demonstrates reliable data throughput, verified by a self-checking testbench comparing all 4096 samples.
* **Data Packing**: The controller correctly packs 12-bit samples from Channel 0 and Channel 1 into a single 32-bit word for efficient memory utilization.
    #### Data Word Mapping
    Each 32-bit word in BRAM contains two ADC samples:
    | Bits  | [31:28] | [27:16]       | [15:12] | [11:0]        |
    |-------|---------|---------------|---------|---------------|
    | Field | Unused  | **Channel 1** | Unused  | **Channel 0** |
    | Value | 0000    | 12-bit sample | 0000    | 12-bit sample |
* **Memory Verification**: Verification of the RAM contents confirms the storage of **4096 samples**. The first entry at address `0x0400` was verified as `0x05550AAA`, and the final entry at `0x13FF` was `0x05540AA9`, matching the expected 12-bit incremental wrap-around logic.

<p align="center">
    <img width=600 src="0.doc/top_tb_behav1.png">
    <br><em>Acquisition Start Sequence</em>
</p>

### 3. Automatic Completion (RUNNING -> DONE -> IDLE)
The controller features an automated stop mechanism to prevent memory overflow.
* **End-of-Buffer Logic**: When the address counter reaches the terminal address `0x13FF`, the acquisition sequence completes.
* **State Reset**: The FSM transitions from the **RUNNING** state to **DONE**, then automatically returns to the **IDLE** state, de-asserting `adc_we_o`.
* **Done Flag**: The `csr_done_o` flag is raised, notifying the CPU that the acquisition buffer is ready for processing via Port A. The internal address is reset to 0x0400, making the module immediately ready for the next trigger.

<p align="center">
    <img width=600 src="0.doc/top_tb_behav2.png">
    <br><em>Buffer Completion and State Reset</em>
</p>

### Automated Verification

To ensure scalability and eliminate manual inspection errors ("eye-balling"), the verification process is fully automated within the `top_tb.sv` environment.

#### Verification Methodology
* **Real-Time Reference Model**: A "gold-standard" array (`expected_mem`) is populated in real-time as data is streamed to the controller, serving as a bit-accurate reference.
* **Automated Post-Acquisition Comparison**: Upon completion (`csr_done_o == 1`), a verification loop automatically iterates through all **4096 samples** in the RAM, comparing them against the reference model.
* **Immediate Result Reporting**: The testbench logs any mismatches and prints a final **PASS/FAIL** summary to the simulation console, replacing the need for manual waveform analysis.

#### Testing via Terminal
The design is set up for quick re-verification after any logic changes. The simulation can be executed directly from the terminal without opening the Vivado GUI, saving time during development.

**Execution Directory:**
`uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.sim/sim_1/behav/xsim`

**Terminal Command:**
```bash
# Runs the simulation in batch mode and returns the verification result
xsim top_tb_behav -R
```
---

### Performance & Results
With this method, the theoretical Nyquist limit is **32.5 MHz**. Experimental results in the lab, using a high-frequency function generator, confirmed successful reconstruction of signals up to **25 MHz** (limited only by the available lab equipment).

<p align="center">
    <img width=600 src="0.doc/DPRAM_ADC_15_25MHz.png">
    <br><em>Acquisition of 15 MHz and 25 MHz sine waves</em>
</p>

#### Key Advantages
* **Max recordable signal:** Increased from 250 kHz to 32.5 MHz (**130x improvement**).
* **Scalability:** Serve as a blueprint for upcoming DAC integration and continuous signal generation.

### 4. DMA-Based DAC Continuous Generation
- WIP

---
#### End of Document
