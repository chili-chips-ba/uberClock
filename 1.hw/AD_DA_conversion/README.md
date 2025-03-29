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
To test the system we generate a sine wave and a square signal using a signal generator, we sample the signal annd then send the sampled data to an osciloscope. 
We started the test by sending 1[MHz] signals and then we increase the frequency by 1[MHz]. Our goal was to see if our system could generate a good quality sine and square of 10[MHz].
On the pictures below we showcase our test results. The two pictures below show 1[MHz] and 10[MHz] sine waves.

<p align="center">
  <img src="0.doc/Sine_1MHz.jpg">
  <img src="0.doc/Sine_10MHz.jpg">
</p>

Now we present our results for the square wave test, on the two pictures below you can see the sampled and generated 1 and 10 [MHz] square signals.

<p align="center">
  <img src="0.doc/Square_1MHz.jpg">
  <img src="0.doc/Square_10MHz.jpg">
</p>

As can be seen from the photos, the larger the frequency, the more noise we got. Since our A/D converter can sample with a speed of 65[Msps] and our D/A converter sends 125[Msps], we presume that the occurring noise is a product of the two domains not being synchronized.
