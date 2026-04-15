Code Layout
===========

The firmware is located under ``2.soc/2.sw/uberclock`` and is split into
headers, source files, and a bundled FFT library.

Directory structure
-------------------

- ``inc/uberclock`` -- public headers (API surface)
- ``src`` -- implementation files
- ``kissfft`` -- FFT library used for spectral analysis

Source files
------------

``main.c``
~~~~~~~~~~

Entry point of the firmware.

- initializes UART and interrupts
- sets up the console
- initializes the uberClock system
- runs the main loop:

.. code-block:: c

   while (1) {
       console_poll();
       uberclock_poll();
   }

``uberclock.c``
~~~~~~~~~~~~~~~

Main runtime core.

- holds global state
- initializes subsystems
- handles CE event-driven processing
- pushes/pulls samples in runtime loop

``uberclock_commands.c``
~~~~~~~~~~~~~~~~~~~~~~~~

UART command interface.

- parses user input
- maps commands to actions
- writes to CSRs or calls runtime helpers

Examples of commands:

- phase / frequency control
- gain and routing
- FFT execution
- tracking start/stop
- capture control

``uberclock_hw.c``
~~~~~~~~~~~~~~~~~~

Low-level hardware interface.

- wraps LiteX CSR access
- converts Hz ↔ phase increment
- commits configuration to hardware

This is the closest layer to the FPGA.

``uberclock_channels.c``
~~~~~~~~~~~~~~~~~~~~~~~~

Per-channel configuration.

- downconversion phase
- CPU-generated tones
- per-channel gain

Maps logical channel index → CSR registers.

``uberclock_fifo.c``
~~~~~~~~~~~~~~~~~~~~

FIFO interaction.

- pop downsampled samples
- push samples into upsampler
- check status and flush FIFOs

Used for streaming data through the DSP path.

``uberclock_capture.c``
~~~~~~~~~~~~~~~~~~~~~~~

Low-speed capture.

- start capture
- check completion
- read/dump captured samples

Used mainly for debugging.

``uberclock_dma.c``
~~~~~~~~~~~~~~~~~~~

DDR / DMA interface.

- start memory capture
- wait for completion
- dump memory
- send data via UDP

Used for large data transfers.

``uberclock_fft.c``
~~~~~~~~~~~~~~~~~~~

FFT processing using KISS FFT.

- capture samples from FIFO
- run FFT
- compute bin and band power

Used by tracking and analysis commands.

``uberclock_track.c``
~~~~~~~~~~~~~~~~~~~~~

Tracking algorithms.

- ``track3``: sweep search
- ``trackq``: closed-loop tracking

Uses FFT results to adjust system frequency in real time.

``uberclock_siggen.c``
~~~~~~~~~~~~~~~~~~~~~~

Software signal generator.

- generates test tones
- pushes samples into datapath

Useful for bring-up without external signals.

``uberclock_parse.c``
~~~~~~~~~~~~~~~~~~~~~

Command parsing helpers.

- integer parsing
- bounds checking
- IPv4 parsing

Used by command handlers.

Notes
-----

- Headers in ``inc/uberclock`` define the API
- Source files in ``src`` implement behavior
- Most interaction happens through the command layer
