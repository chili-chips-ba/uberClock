
# Create Project

create_project -force -name alinx_ax7203 -part xc7a200tfbg484-2
set_msg_config -id {Common 17-55} -new_severity {Warning}

# Add project commands


# Add Sources

read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/ddr3_top.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/ddr3_controller.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/ddr3_phy.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/wbc2pipeline.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/wbxbar.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/skidbuffer.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/addrdecode.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/memory/zipdma_s2mm.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/uberclock/uberclock.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/uberclock/rx_channel.v}
read_verilog {/home/ahmed/ws/uberClock/2.soc/1.hw/uberclock/tx_channel.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/adc/adc.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/dac/dac.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/cic.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/cic_comp_down_mac.v}
add_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/comp_down_coeffs.mem}
if {[string equal [file extension /home/ahmed/ws/uberClock/1.dsp/rtl/filters/comp_down_coeffs.mem] ".h"]} {
set_property is_global_include true [get_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/comp_down_coeffs.mem}]
}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/hb_down_mac.v}
add_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/hb_down_coeffs.mem}
if {[string equal [file extension /home/ahmed/ws/uberClock/1.dsp/rtl/filters/hb_down_coeffs.mem] ".h"]} {
set_property is_global_include true [get_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/hb_down_coeffs.mem}]
}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/downsamplerFilter.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/upsamplerFilter.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/hb_up_mac.v}
add_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs.mem}
if {[string equal [file extension /home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs.mem] ".h"]} {
set_property is_global_include true [get_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs.mem}]
}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/cic_comp_up_mac.v}
add_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs_comp.mem}
if {[string equal [file extension /home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs_comp.mem] ".h"]} {
set_property is_global_include true [get_files {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/coeffs_comp.mem}]
}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/filters/cic_int.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/to_polar/to_polar.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/cordic_pre_rotate.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/cordic_pipeline_stage.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/cordic_round.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/cordic.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/cordic_logic.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic/gain_and_saturate.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic16/cordic16.v}
read_verilog {/home/ahmed/ws/uberClock/1.dsp/rtl/cordic16/cordic_pre_rotate_16.v}
read_verilog {/home/ahmed/litex/pythondata-cpu-vexriscv/pythondata_cpu_vexriscv/verilog/VexRiscv.v}
read_verilog {/home/ahmed/ws/uberClock/3.build/build/alinx_ax7203/gateware/alinx_ax7203.v}

# Add EDIFs


# Add IPs


# Add constraints

read_xdc alinx_ax7203.xdc
set_property PROCESSING_ORDER EARLY [get_files alinx_ax7203.xdc]

# Add pre-synthesis commands


# Synthesis

synth_design -directive default -top alinx_ax7203 -part xc7a200tfbg484-2

# Synthesis report

report_timing_summary -file alinx_ax7203_timing_synth.rpt
report_utilization -hierarchical -file alinx_ax7203_utilization_hierarchical_synth.rpt
report_utilization -file alinx_ax7203_utilization_synth.rpt
write_checkpoint -force alinx_ax7203_synth.dcp

# Add pre-optimize commands


# Optimize design

opt_design -directive default

# Add pre-placement commands


# Placement

place_design -directive default

# Placement report

report_utilization -hierarchical -file alinx_ax7203_utilization_hierarchical_place.rpt
report_utilization -file alinx_ax7203_utilization_place.rpt
report_io -file alinx_ax7203_io.rpt
report_control_sets -verbose -file alinx_ax7203_control_sets.rpt
report_clock_utilization -file alinx_ax7203_clock_utilization.rpt
write_checkpoint -force alinx_ax7203_place.dcp

# Add pre-routing commands


# Routing

route_design -directive default
phys_opt_design -directive default
write_checkpoint -force alinx_ax7203_route.dcp

# Routing report

report_timing_summary -no_header -no_detailed_paths
report_route_status -file alinx_ax7203_route_status.rpt
report_drc -file alinx_ax7203_drc.rpt
report_timing_summary -datasheet -max_paths 10 -file alinx_ax7203_timing.rpt
report_power -file alinx_ax7203_power.rpt

# Bitstream generation

write_bitstream -force alinx_ax7203.bit 

# End

quit