# How to build, run and debug the design

Everything is run from `2.soc-litex/3.build` directory.

Running `make help` gives the following output:

```
make help
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
```

## Step 1: Building the design

The design currently has different parts that can be built separately using flags. To run the build use the following command:
```
make build-board FREQ=65000000 OPTIONS="--with-etherbone --with-adc-cordic-dsp-dac"
```

## Step 2: Build the software part of the design

When the synthesis and implementation are done, only then you can build the software part using:
```
make build-sw
```

## Step 3: Run the UART terminal command

If you wish to run the serial terminal to control the CPU via UART do the following.

1. Open up a terminal and run the command:
```
make term
```

**NOTE**: Sometimes you need to change the port and you can do that by using `PORT=/devttyUSBx`

2. Open another terminal **WHILE** the serial terminal is open and load the design with:
```
make load
```

Wait for the design to load and in the serial you should see the `uberClock` design.


## Step 4: Debugging

**NOTE**: Source your `litex-venv` first. You also need to build your design with `--with-etherbone`.
If you wish to debug using `litescope` you also need the following commands open in two different terminals:
```
litex_server --udp --udp-ip=192.168.1.123
```

```
litescope_cli --host=localhost --port=1234 --csr-csv=build/alinx_ax7203/csr.csv --csv=analyzer.csv --dump=dump.vcd
```


