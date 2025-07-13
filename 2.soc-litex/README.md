# SoC-Based Control with LiteX & VexRiscV

This project uses [LiteX](https://github.com/enjoy-digital/litex) as the FPGA framework and instantiates a [VexRiscV](https://github.com/SpinalHDL/VexRiscv) soft-core to control all of the ADC → DSP → DAC flow. While the bulk of the digital signal processing pipeline lives in parameterized Verilog modules (ADCs, filters, CORDIC NCOs, DACs), the CPU handles all of the configuration and data movement at run-time via a simple UART console interface. 

## Workflow Overview
Next up, let’s dive into the 2.soc-litex folder and walk through its structure so you can see how each component fits together and how the overall data-path control flow comes together.

### 1.hw
In the [1.hw](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/1.hw) folder you'll find every Verilog block organized into its own respective subdirectory. Each piece—ADC, CORDIC, CIC filters, DAC—has been individually tested and simulated, and they’re all built to plug right into a larger module. We won’t dig into the nitty-gritty of each hardware implementation here; instead, we will be focused on how LiteX brings it all together and puts you in control of the whole data path.

### 2.sw

In the [2.sw](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/1.hw) is a modified version of the [LiteX Demo Application](https://github.com/enjoy-digital/litex/tree/master/litex/soc/software/demo) that includes the generated csr from the project build in the `main.c` function. There lie the control function for the datapath in bare-metal C.
The uart console is built upon the official demo application. 

### 3.build

Here in the [3.build](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/3.build) folder we have the [Makefile](https://github.com/chili-chips-ba/uberClock/blob/main/2.soc-litex/3.build/Makefile) that tries to ease the LiteX workflow into different parts. Running `make help` gives pretty self-exmplanatory commands:

```
$ make help
Usage:
  make open-target    file=<name>
  make open-example   file=<proj>
  make build-board    [FREQ=..] [OPTIONS='..']
  make load           [FREQ=..] [OPTIONS='..']
  make term                [PORT=..]
  make build-sw            [DEMO_FLAGS='..']
  make sim
  make view-sim
  make clean-sim
  make setup-ethernet      [ETH_IFACE=..] [STATIC_IP=..]
  make start-server
  make stop-server
  make litescope
  make clean
  make copy-migen
```

The most important ones are:

* `make build-board` which runs the build process using Vivado(later openXC7 will be used instead). The command has parameters `FREQ` and `OPTIONS` which can be overloaded, but running it without any parameters is going to include `--with-etherbone` and `--with-uberclock` with a frequency of `65MHz`.

* `make build-sw` which compiles the bare-metal C application. The imporant note to add is to run the command after the build finishes because the compiler depends of the generated files in the `3.build/build` folder.

* `make load` simply loads the bitstream onto the FPGA via the JTAG programmer
  
* `make term` opens up the serial console in the terminal to control the datapth signals. The default port is `/dev/ttyUSB0` while that can be simply changed with `PORT`.

**NOTE**: Run `make term` in a seperate terminal before running `make load`

### 5.docker
[5.docker](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/5.docker) is still in development. The tough part is adding Vivado into the Docker container.

### 6.migen
The [6.migen](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/6.migen) folder contains two subfolders:
* [platforms](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/6.migen/platforms) which has the specific platform file that specifies the pins of the Alinx AX7203 board.
* [targets](https://github.com/chili-chips-ba/uberClock/tree/main/2.soc-litex/6.migen/targets) has the target file that specifies all of the important parts of the CPU design and instantiates the SoC with the used Verilog files.

**NOTE**: The target and platform files so far haven't been added to the offical LiteX repository. The plaform files will eventually be added while the target file will be added but without the uberClock project specific parts of course. Right now you need to copy these subfolders with the files manually in your LiteX installation folder in `litex/litex_board/litex-boards`. Or go to the `Makefile` and run `make copy-migen` but keep in mind to specify the `LITEX_PATH` to your path in the `Makefile`.


What is also important to add with the workflow is the use of [Litescope](https://github.com/chili-chips-ba/uberClock/tree/main/1.dsp) to see and debug signals in real time. The use of this will be explained under a different README.

## CPU-Driven Data-Path Control

Using the CPU with the UART console, we can dynamically steer every stage of the signal chain without touching the FPGA bitstream.
Running the `help` command in the UART console gives the following output:
```
Available commands:
  help                  - Show this command
  reboot                - Reboot CPU
  phase_nco <val>      - Set input CORDIC NCO phase increment (0–524287)
  phase_down <val>     - Set downconversion CORDIC phase increment (0–524287)
  output_select_ch1 <val>  - Choose DAC1 (channel 1) output source:
                           0 = downsampledY
                           1 = upsampledX
                           2 = y_downconverted
                           3 = y_upconverted
  output_select_ch2 <val>  - Choose DAC2 (channel 2) output source:
                           0 = upsampledY
                           1 = filter_in
                           2 = nco_cos
                           3 = upsampledY
  input_select <val>   - Set main input select register (0=ADC, 1=NCO)
  gain1 <val>           - Set Gain1 register (Q format value)
  gain2 <val>           - Set Gain2 register (Q format value)
```
The commands are explained further below:

* `input_select` : choose between the external ADC → DSP → DAC path or an internally generated sinewave from the CORDIC NCO.
* `phase_nco` and `phase_down` : adjust the CORDIC phase increment on-the-fly
* `output_select_ch1` and `output_select_ch2` : remap any internal node (down-converted, down-sampled, up-converted, etc.) to either DAC channel at run-time.
* `gain1` and `gain2` sets the gain values the the specific channels.
**NOTE**: Right now only one channel is viable while we are working on making a modular N-channel module so we can instantate N-channels.

What was done without the UART console is to read samples from the downsampler into the CPU, apply custom processing (e.g. scaling or math), then feed them back into the upsampler path. Three polling methods were evaluated and the specific details on the implementations in LiteX will be explained in another README:

* Simple while-loop that blocks the UART and is deemed undesirable

* Periodic timer (10 kHz) (works, but adds fixed latency)

* Event-driven interrupt (triggers exactly on new data—optimal)

Here we can see two pictures that show the event-driven interrupt where the CPU doubles the amplitude of the signal(downsampled/upsampled) that it recieves.

<img width="1024" height="630" alt="twox0" src="https://github.com/user-attachments/assets/24d399f1-58c5-441e-a952-37cd817eb073" />

The first picture above shows the downsampled signal that is read by the CPU in yellow while in blue it shows the doubled signal that is passed to the DAC to show that the data is being read and modified by the CPU.

<img width="1024" height="630" alt="inouttenmhz0" src="https://github.com/user-attachments/assets/172b477b-097d-47bf-a1a7-ae6d325b1343" />

The second picture shows the same but for the signal that is being sent by the CPU to the upsampler. It is also being doubled in the CPU to show that it is being read and manipulated accordingly.


In the process of making this top level uberclock module we have evaluated the specific Verilog modules and their simulations extensively and that part is going to be described in [1.dsp](https://github.com/chili-chips-ba/uberClock/tree/main/1.dsp).
