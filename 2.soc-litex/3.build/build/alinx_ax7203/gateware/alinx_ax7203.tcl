
# Create Project

create_project -force -name alinx_ax7203 -part xc7a200tfbg484-2
set_msg_config -id {Common 17-55} -new_severity {Warning}

# Add project commands


# Add Sources

read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/cordic/cordic_pre_rotate.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/cordic/cordic_pipeline_stage.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/cordic/cordic_round.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/cordic/cordic.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/dac/dac.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/1.hw/cordic-dac/cordic_dac.v}
read_verilog {/home/hamed/FPGA/Tools/litex-hub/litex/pythondata-cpu-vexriscv/pythondata_cpu_vexriscv/verilog/VexRiscv.v}
read_verilog {/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/3.build/build/alinx_ax7203/gateware/alinx_ax7203.v}

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