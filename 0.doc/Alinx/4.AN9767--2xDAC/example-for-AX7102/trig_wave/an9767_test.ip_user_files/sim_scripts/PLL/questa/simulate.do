onbreak {quit -f}
onerror {quit -f}

vsim -t 1ps -lib xil_defaultlib PLL_opt

do {wave.do}

view wave
view structure
view signals

do {PLL.udo}

run -all

quit -force
