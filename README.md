# uberClock

Clocks can be extracted from GPS satellite signals, or locally generated with MEMS oscillators, SAW resonators, quartz crystal (XTAL, XO) or piezo resonators, often set in “ovens” (TCXO, OCXO), derived from atomic properties (like Cesium Beam, Hydrogen Maser, Rubidium, Strontium or Ytterbium), or obtained in another way.

<p align="center">
  <img width=300 src="0.doc/uberclock.logo.png">
</p>

They differ in Absolute Accuracy, Long-term Frequency Stability (e.g. due to aging), Short-term Frequency Stability (due to temperature changes), Phase Noise (aka Jitter), physical size, complexity, immunity to external interference (due to physical vibrations, humidity, EMP, EW), power consumption, cost, etc. These differences are categorized as “Clock Strata”, whereby a clock source must meet a standardized set of requirements for each Stratum level.

This work is about researching and exploiting the properties of multi-mode crystal oscillators in order to achieve stability comparable to a Stratum 2 Rubidium clock, all at a fraction of the total cost of ownership. We plan on collecting large empirical datasets, constructing experimental prototypes, and using DSP / numerical methods to mitigate temperature variations, dynamic acceleration and static gravity effects. The project aims for XTAL frequency stability by means of numerous mathematical calculations performed in FPGA, using open-source tools, including CflexHDL+PipelineC HLS flow.

This is a Proof-of-Concept (PoC) and stepping stone for future applied research projects on this theme, possibly extending into the field of Artificial Intelligence. In addition to a working prototype (PCBs, FPGA Gateware and Embedded Firmware), the project will deliver a series of scientific papers.

### Acknowledgements
We are grateful to NLnet Foundation for their sponsorship of this development activity.

<p align="center">
   <img src="https://github.com/chili-chips-ba/openeye/assets/67533663/18e7db5c-8c52-406b-a58e-8860caa327c2">
   <img width="115" alt="NGI-Entrust-Logo" src="https://github.com/chili-chips-ba/openeye-CamSI/assets/67533663/013684f5-d530-42ab-807d-b4afd34c1522">
</p>

### Public posts:
- Soon to come

#### End of Document

