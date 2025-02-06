**1.rtl** holds the source files.

**2.sim** holds the  testbeches for cordic and cordic based downconversion modules.

**cordic.v** implements a cordic in rotation mode. When inputs are set to X=const. and Y = 0, cordic serves as a DDS. In this example a 1MHz sinewave is synthesized.

When inputs are set to X = 0, Y = sin(wc t), cordic serves as a mixer for downconversion.
In this example, 1 MHz sinewave is mixed with 900 kHz. After lowpass CIC filter (**cic.v**)**,** the downconverted 100 kHz signal is obtained at 64x decimated sampling rate.

![Screenshot from 2025-02-05 21-50-38](https://github.com/user-attachments/assets/d67a455d-1f42-43a5-8563-e40613d3d251)
