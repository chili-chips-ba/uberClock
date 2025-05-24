# AD/DA conversion

## Overview
In this section we present the RTL we have written to interface the AX7203 FPGA board with the AN9238 A/D and AN9767 D/A converters.
On the picture below you can see the FPGA board connected to the A/D and D/A converters.

<p align="center">
  <img src="0.doc/AD_DA_system.jpg">
</p>

## Implementation
Since we are using a 12-bit A/D converter and our D/A converter uses 14-bits we had to shift the sampled data up by two bits.

## Lab results
To test the system we generate a sine wave and a square signal using a signal generator, we sample the signal and then send the sampled data to an osciloscope. 
We started the test by sending 1[MHz] signals and then we increase the frequency by 1[MHz]. Our goal was to see if our system could generate a good quality sine and square of 10[MHz].
On the pictures below we showcase our test results. The picture below shows two signals: the yellow sine shows the sampled signal and the blue sine shows the generated signal.

<p align="center">
  <img src="0.doc/osc_adc_fpga_dac_osc.png">
</p>
