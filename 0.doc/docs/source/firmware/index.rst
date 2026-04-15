Firmware
========

The firmware under ``2.soc/2.sw`` runs on the VexRiscV CPU and provides runtime
control over the ADC → DSP → DAC datapath.

It allows:

- configuring DSP blocks via CSRs,
- interacting with the system through a UART console,
- capturing and inspecting data,
- running FFT analysis,
- executing tracking algorithms.

The firmware turns the hardware into an interactive system instead of a fixed
bitstream.

.. toctree::
   :maxdepth: 1

   code_layout
   c_api
