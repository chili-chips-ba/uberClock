################################################################################
# IO constraints
################################################################################
# clk200:0.p
set_property LOC R4 [get_ports {clk200_p}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {clk200_p}]

# clk200:0.n
set_property LOC T4 [get_ports {clk200_n}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {clk200_n}]

# serial:0.tx
set_property LOC N15 [get_ports {serial_tx}]
set_property IOSTANDARD LVCMOS33 [get_ports {serial_tx}]

# serial:0.rx
set_property LOC P20 [get_ports {serial_rx}]
set_property IOSTANDARD LVCMOS33 [get_ports {serial_rx}]

# user_led:0
set_property LOC B13 [get_ports {user_led0}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led0}]

# user_led:1
set_property LOC C13 [get_ports {user_led1}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led1}]

# user_led:2
set_property LOC D14 [get_ports {user_led2}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led2}]

# user_led:3
set_property LOC D15 [get_ports {user_led3}]
set_property IOSTANDARD LVCMOS33 [get_ports {user_led3}]

# ddram:0.a
set_property LOC AA4 [get_ports {ddram_a[0]}]
set_property SLEW FAST [get_ports {ddram_a[0]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[0]}]

# ddram:0.a
set_property LOC AB2 [get_ports {ddram_a[1]}]
set_property SLEW FAST [get_ports {ddram_a[1]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[1]}]

# ddram:0.a
set_property LOC AA5 [get_ports {ddram_a[2]}]
set_property SLEW FAST [get_ports {ddram_a[2]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[2]}]

# ddram:0.a
set_property LOC AB5 [get_ports {ddram_a[3]}]
set_property SLEW FAST [get_ports {ddram_a[3]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[3]}]

# ddram:0.a
set_property LOC AB1 [get_ports {ddram_a[4]}]
set_property SLEW FAST [get_ports {ddram_a[4]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[4]}]

# ddram:0.a
set_property LOC U3 [get_ports {ddram_a[5]}]
set_property SLEW FAST [get_ports {ddram_a[5]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[5]}]

# ddram:0.a
set_property LOC W1 [get_ports {ddram_a[6]}]
set_property SLEW FAST [get_ports {ddram_a[6]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[6]}]

# ddram:0.a
set_property LOC T1 [get_ports {ddram_a[7]}]
set_property SLEW FAST [get_ports {ddram_a[7]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[7]}]

# ddram:0.a
set_property LOC V2 [get_ports {ddram_a[8]}]
set_property SLEW FAST [get_ports {ddram_a[8]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[8]}]

# ddram:0.a
set_property LOC U2 [get_ports {ddram_a[9]}]
set_property SLEW FAST [get_ports {ddram_a[9]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[9]}]

# ddram:0.a
set_property LOC Y1 [get_ports {ddram_a[10]}]
set_property SLEW FAST [get_ports {ddram_a[10]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[10]}]

# ddram:0.a
set_property LOC W2 [get_ports {ddram_a[11]}]
set_property SLEW FAST [get_ports {ddram_a[11]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[11]}]

# ddram:0.a
set_property LOC Y2 [get_ports {ddram_a[12]}]
set_property SLEW FAST [get_ports {ddram_a[12]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[12]}]

# ddram:0.a
set_property LOC U1 [get_ports {ddram_a[13]}]
set_property SLEW FAST [get_ports {ddram_a[13]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[13]}]

# ddram:0.a
set_property LOC V3 [get_ports {ddram_a[14]}]
set_property SLEW FAST [get_ports {ddram_a[14]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_a[14]}]

# ddram:0.ba
set_property LOC AA3 [get_ports {ddram_ba[0]}]
set_property SLEW FAST [get_ports {ddram_ba[0]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_ba[0]}]

# ddram:0.ba
set_property LOC Y3 [get_ports {ddram_ba[1]}]
set_property SLEW FAST [get_ports {ddram_ba[1]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_ba[1]}]

# ddram:0.ba
set_property LOC Y4 [get_ports {ddram_ba[2]}]
set_property SLEW FAST [get_ports {ddram_ba[2]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_ba[2]}]

# ddram:0.ras_n
set_property LOC V4 [get_ports {ddram_ras_n}]
set_property SLEW FAST [get_ports {ddram_ras_n}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_ras_n}]

# ddram:0.cas_n
set_property LOC W4 [get_ports {ddram_cas_n}]
set_property SLEW FAST [get_ports {ddram_cas_n}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_cas_n}]

# ddram:0.we_n
set_property LOC AA1 [get_ports {ddram_we_n}]
set_property SLEW FAST [get_ports {ddram_we_n}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_we_n}]

# ddram:0.cs_n
set_property LOC AB3 [get_ports {ddram_cs_n}]
set_property SLEW FAST [get_ports {ddram_cs_n}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_cs_n}]

# ddram:0.dm
set_property LOC D2 [get_ports {ddram_dm[0]}]
set_property SLEW FAST [get_ports {ddram_dm[0]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dm[0]}]

# ddram:0.dm
set_property LOC G2 [get_ports {ddram_dm[1]}]
set_property SLEW FAST [get_ports {ddram_dm[1]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dm[1]}]

# ddram:0.dm
set_property LOC M2 [get_ports {ddram_dm[2]}]
set_property SLEW FAST [get_ports {ddram_dm[2]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dm[2]}]

# ddram:0.dm
set_property LOC M5 [get_ports {ddram_dm[3]}]
set_property SLEW FAST [get_ports {ddram_dm[3]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dm[3]}]

# ddram:0.dq
set_property LOC C2 [get_ports {ddram_dq[0]}]
set_property SLEW FAST [get_ports {ddram_dq[0]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[0]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[0]}]

# ddram:0.dq
set_property LOC G1 [get_ports {ddram_dq[1]}]
set_property SLEW FAST [get_ports {ddram_dq[1]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[1]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[1]}]

# ddram:0.dq
set_property LOC A1 [get_ports {ddram_dq[2]}]
set_property SLEW FAST [get_ports {ddram_dq[2]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[2]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[2]}]

# ddram:0.dq
set_property LOC F3 [get_ports {ddram_dq[3]}]
set_property SLEW FAST [get_ports {ddram_dq[3]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[3]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[3]}]

# ddram:0.dq
set_property LOC B2 [get_ports {ddram_dq[4]}]
set_property SLEW FAST [get_ports {ddram_dq[4]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[4]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[4]}]

# ddram:0.dq
set_property LOC F1 [get_ports {ddram_dq[5]}]
set_property SLEW FAST [get_ports {ddram_dq[5]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[5]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[5]}]

# ddram:0.dq
set_property LOC B1 [get_ports {ddram_dq[6]}]
set_property SLEW FAST [get_ports {ddram_dq[6]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[6]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[6]}]

# ddram:0.dq
set_property LOC E2 [get_ports {ddram_dq[7]}]
set_property SLEW FAST [get_ports {ddram_dq[7]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[7]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[7]}]

# ddram:0.dq
set_property LOC H3 [get_ports {ddram_dq[8]}]
set_property SLEW FAST [get_ports {ddram_dq[8]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[8]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[8]}]

# ddram:0.dq
set_property LOC G3 [get_ports {ddram_dq[9]}]
set_property SLEW FAST [get_ports {ddram_dq[9]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[9]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[9]}]

# ddram:0.dq
set_property LOC H2 [get_ports {ddram_dq[10]}]
set_property SLEW FAST [get_ports {ddram_dq[10]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[10]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[10]}]

# ddram:0.dq
set_property LOC H5 [get_ports {ddram_dq[11]}]
set_property SLEW FAST [get_ports {ddram_dq[11]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[11]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[11]}]

# ddram:0.dq
set_property LOC J1 [get_ports {ddram_dq[12]}]
set_property SLEW FAST [get_ports {ddram_dq[12]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[12]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[12]}]

# ddram:0.dq
set_property LOC J5 [get_ports {ddram_dq[13]}]
set_property SLEW FAST [get_ports {ddram_dq[13]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[13]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[13]}]

# ddram:0.dq
set_property LOC K1 [get_ports {ddram_dq[14]}]
set_property SLEW FAST [get_ports {ddram_dq[14]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[14]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[14]}]

# ddram:0.dq
set_property LOC H4 [get_ports {ddram_dq[15]}]
set_property SLEW FAST [get_ports {ddram_dq[15]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[15]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[15]}]

# ddram:0.dq
set_property LOC L4 [get_ports {ddram_dq[16]}]
set_property SLEW FAST [get_ports {ddram_dq[16]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[16]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[16]}]

# ddram:0.dq
set_property LOC M3 [get_ports {ddram_dq[17]}]
set_property SLEW FAST [get_ports {ddram_dq[17]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[17]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[17]}]

# ddram:0.dq
set_property LOC L3 [get_ports {ddram_dq[18]}]
set_property SLEW FAST [get_ports {ddram_dq[18]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[18]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[18]}]

# ddram:0.dq
set_property LOC J6 [get_ports {ddram_dq[19]}]
set_property SLEW FAST [get_ports {ddram_dq[19]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[19]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[19]}]

# ddram:0.dq
set_property LOC K3 [get_ports {ddram_dq[20]}]
set_property SLEW FAST [get_ports {ddram_dq[20]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[20]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[20]}]

# ddram:0.dq
set_property LOC K6 [get_ports {ddram_dq[21]}]
set_property SLEW FAST [get_ports {ddram_dq[21]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[21]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[21]}]

# ddram:0.dq
set_property LOC J4 [get_ports {ddram_dq[22]}]
set_property SLEW FAST [get_ports {ddram_dq[22]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[22]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[22]}]

# ddram:0.dq
set_property LOC L5 [get_ports {ddram_dq[23]}]
set_property SLEW FAST [get_ports {ddram_dq[23]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[23]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[23]}]

# ddram:0.dq
set_property LOC P1 [get_ports {ddram_dq[24]}]
set_property SLEW FAST [get_ports {ddram_dq[24]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[24]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[24]}]

# ddram:0.dq
set_property LOC N4 [get_ports {ddram_dq[25]}]
set_property SLEW FAST [get_ports {ddram_dq[25]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[25]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[25]}]

# ddram:0.dq
set_property LOC R1 [get_ports {ddram_dq[26]}]
set_property SLEW FAST [get_ports {ddram_dq[26]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[26]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[26]}]

# ddram:0.dq
set_property LOC N2 [get_ports {ddram_dq[27]}]
set_property SLEW FAST [get_ports {ddram_dq[27]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[27]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[27]}]

# ddram:0.dq
set_property LOC M6 [get_ports {ddram_dq[28]}]
set_property SLEW FAST [get_ports {ddram_dq[28]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[28]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[28]}]

# ddram:0.dq
set_property LOC N5 [get_ports {ddram_dq[29]}]
set_property SLEW FAST [get_ports {ddram_dq[29]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[29]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[29]}]

# ddram:0.dq
set_property LOC P6 [get_ports {ddram_dq[30]}]
set_property SLEW FAST [get_ports {ddram_dq[30]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[30]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[30]}]

# ddram:0.dq
set_property LOC P2 [get_ports {ddram_dq[31]}]
set_property SLEW FAST [get_ports {ddram_dq[31]}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_dq[31]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dq[31]}]

# ddram:0.dqs_p
set_property LOC E1 [get_ports {ddram_dqs_p[0]}]
set_property SLEW FAST [get_ports {ddram_dqs_p[0]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_p[0]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_p[0]}]

# ddram:0.dqs_p
set_property LOC K2 [get_ports {ddram_dqs_p[1]}]
set_property SLEW FAST [get_ports {ddram_dqs_p[1]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_p[1]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_p[1]}]

# ddram:0.dqs_p
set_property LOC M1 [get_ports {ddram_dqs_p[2]}]
set_property SLEW FAST [get_ports {ddram_dqs_p[2]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_p[2]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_p[2]}]

# ddram:0.dqs_p
set_property LOC P5 [get_ports {ddram_dqs_p[3]}]
set_property SLEW FAST [get_ports {ddram_dqs_p[3]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_p[3]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_p[3]}]

# ddram:0.dqs_n
set_property LOC D1 [get_ports {ddram_dqs_n[0]}]
set_property SLEW FAST [get_ports {ddram_dqs_n[0]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_n[0]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_n[0]}]

# ddram:0.dqs_n
set_property LOC J2 [get_ports {ddram_dqs_n[1]}]
set_property SLEW FAST [get_ports {ddram_dqs_n[1]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_n[1]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_n[1]}]

# ddram:0.dqs_n
set_property LOC L1 [get_ports {ddram_dqs_n[2]}]
set_property SLEW FAST [get_ports {ddram_dqs_n[2]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_n[2]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_n[2]}]

# ddram:0.dqs_n
set_property LOC P4 [get_ports {ddram_dqs_n[3]}]
set_property SLEW FAST [get_ports {ddram_dqs_n[3]}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_dqs_n[3]}]
set_property IN_TERM UNTUNED_SPLIT_50 [get_ports {ddram_dqs_n[3]}]

# ddram:0.clk_p
set_property LOC R3 [get_ports {ddram_clk_p}]
set_property SLEW FAST [get_ports {ddram_clk_p}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_clk_p}]

# ddram:0.clk_n
set_property LOC R2 [get_ports {ddram_clk_n}]
set_property SLEW FAST [get_ports {ddram_clk_n}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {ddram_clk_n}]

# ddram:0.cke
set_property LOC T5 [get_ports {ddram_cke}]
set_property SLEW FAST [get_ports {ddram_cke}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_cke}]

# ddram:0.odt
set_property LOC U5 [get_ports {ddram_odt}]
set_property SLEW FAST [get_ports {ddram_odt}]
set_property IOSTANDARD SSTL15 [get_ports {ddram_odt}]

# ddram:0.reset_n
set_property LOC W6 [get_ports {ddram_reset_n}]
set_property SLEW FAST [get_ports {ddram_reset_n}]
set_property IOSTANDARD LVCMOS15 [get_ports {ddram_reset_n}]

# da1_clk:0
set_property LOC Y17 [get_ports {da1_clk}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_clk}]

# da1_wrt:0
set_property LOC T20 [get_ports {da1_wrt}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_wrt}]

# da1_data:0
set_property LOC AB18 [get_ports {da1_data[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[0]}]

# da1_data:0
set_property LOC AA18 [get_ports {da1_data[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[1]}]

# da1_data:0
set_property LOC AA19 [get_ports {da1_data[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[2]}]

# da1_data:0
set_property LOC AB20 [get_ports {da1_data[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[3]}]

# da1_data:0
set_property LOC AA20 [get_ports {da1_data[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[4]}]

# da1_data:0
set_property LOC AA21 [get_ports {da1_data[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[5]}]

# da1_data:0
set_property LOC AB21 [get_ports {da1_data[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[6]}]

# da1_data:0
set_property LOC AB22 [get_ports {da1_data[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[7]}]

# da1_data:0
set_property LOC V15 [get_ports {da1_data[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[8]}]

# da1_data:0
set_property LOC U15 [get_ports {da1_data[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[9]}]

# da1_data:0
set_property LOC W17 [get_ports {da1_data[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[10]}]

# da1_data:0
set_property LOC V17 [get_ports {da1_data[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[11]}]

# da1_data:0
set_property LOC W15 [get_ports {da1_data[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[12]}]

# da1_data:0
set_property LOC W16 [get_ports {da1_data[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[13]}]

# da2_clk:0
set_property LOC W22 [get_ports {da2_clk}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_clk}]

# da2_wrt:0
set_property LOC W21 [get_ports {da2_wrt}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_wrt}]

# da2_data:0
set_property LOC P14 [get_ports {da2_data[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[0]}]

# da2_data:0
set_property LOC R14 [get_ports {da2_data[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[1]}]

# da2_data:0
set_property LOC R18 [get_ports {da2_data[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[2]}]

# da2_data:0
set_property LOC T18 [get_ports {da2_data[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[3]}]

# da2_data:0
set_property LOC U22 [get_ports {da2_data[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[4]}]

# da2_data:0
set_property LOC V22 [get_ports {da2_data[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[5]}]

# da2_data:0
set_property LOC Y18 [get_ports {da2_data[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[6]}]

# da2_data:0
set_property LOC Y19 [get_ports {da2_data[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[7]}]

# da2_data:0
set_property LOC W19 [get_ports {da2_data[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[8]}]

# da2_data:0
set_property LOC W20 [get_ports {da2_data[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[9]}]

# da2_data:0
set_property LOC Y22 [get_ports {da2_data[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[10]}]

# da2_data:0
set_property LOC Y21 [get_ports {da2_data[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[11]}]

# da2_data:0
set_property LOC U21 [get_ports {da2_data[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[12]}]

# da2_data:0
set_property LOC T21 [get_ports {da2_data[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[13]}]

################################################################################
# Design constraints
################################################################################

################################################################################
# Clock constraints
################################################################################


create_clock -name clk200_p -period 5.0 [get_ports clk200_p]

################################################################################
# False path constraints
################################################################################


set_false_path -quiet -through [get_nets -hierarchical -filter {mr_ff == TRUE}]

set_false_path -quiet -to [get_pins -filter {REF_PIN_NAME == PRE} -of_objects [get_cells -hierarchical -filter {ars_ff1 == TRUE || ars_ff2 == TRUE}]]

set_max_delay 2 -quiet -from [get_pins -filter {REF_PIN_NAME == C} -of_objects [get_cells -hierarchical -filter {ars_ff1 == TRUE}]] -to [get_pins -filter {REF_PIN_NAME == D} -of_objects [get_cells -hierarchical -filter {ars_ff2 == TRUE}]]

set_clock_groups -group [get_clocks -include_generated_clocks -of [get_nets sys_clk]] -group [get_clocks -include_generated_clocks -of [get_nets crg_clkin]] -asynchronous