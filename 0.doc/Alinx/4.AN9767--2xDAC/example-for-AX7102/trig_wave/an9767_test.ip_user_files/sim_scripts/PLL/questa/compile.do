vlib work
vlib msim

vlib msim/xil_defaultlib

vmap xil_defaultlib msim/xil_defaultlib

vlog -work xil_defaultlib -64 \
"../../../../an9767_test.srcs/sources_1/ip/PLL/PLL_clk_wiz.v" \
"../../../../an9767_test.srcs/sources_1/ip/PLL/PLL.v" \


vlog -work xil_defaultlib "glbl.v"

