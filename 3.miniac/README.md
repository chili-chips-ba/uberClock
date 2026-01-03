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

In this section we will present the two implemented methods: Direct and Indirect snooping methods.

### Direct snooping method

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


### Indirect snooping method

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

### 3. DMA-Based ADC Snapshot Method (Hardware Acceleration)

The pinnacle of the Miniac's evolution is the transition from **Processor-In-the-Loop (PIO)** acquisition to a hardware-accelerated **DMA (Direct Memory Access)** approach using **Dual-Port BRAM**. This method shatters the previous 250 kHz limitation, enabling full-speed acquisition at the ADC's native sampling rate of **65 MSPS**.

#### Architecture and Data Flow
In the previous "Indirect Method," the RISC-V CPU was responsible for reading samples from the ADC CSR and writing them to memory. This created a bottleneck due to CPU instruction overhead.

The new DMA ADC Snapshot architecture introduces a dedicated `adc_mem_controller`:

* **Dual-Port RAM Isolation:** Port A is connected to the RISC-V CPU, while Port B is dedicated to the ADC Controller.
* **Hardware Triggering:** When the CPU sets an enable bit, the hardware controller takes absolute control, streaming 12-bit samples at a constant **65 MHz** clock.
* **Zero CPU Overhead:** The CPU is free to perform other tasks while the hardware fills the "snapshot" buffer.
* **Post-Processing:** Once full, the CPU reads data via Port A and transmits it via UART. Since the data is already captured, UART speed no longer affects signal fidelity.

#### System Block Diagram
<p align="center">
  <img width=600 src="0.doc/DMA_ADC_DPRAM.png">
  <br><em>DMA-Based Architecture with Dual-Port BRAM</em>
</p>
The DMA-based architecture consists of three core components:

* **ADC Controller:** Dedicated hardware logic that awaits a "trigger" signal from the CPU and then directly streams 12-bit data from the ADC into memory without processor intervention.
* **Dual-Port BRAM:** A high-speed memory buffer acting as a bridge between clock domains. **Port B** is dedicated exclusively to the ADC (Write-only), while **Port A** is reserved for the CPU (Read-only).
* **Control & Status Registers (CSR):** The command interface through which the CPU initiates acquisition (via the *start* bit) and monitors the status to check if the buffer is full (via the *done* bit).

#### Performance & Results
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
