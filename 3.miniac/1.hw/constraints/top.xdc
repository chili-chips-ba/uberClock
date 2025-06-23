##==========================================================================
## Copyright (C) 2024-2025 Chili.CHIPS*ba
##--------------------------------------------------------------------------
##                      PROPRIETARY INFORMATION
##
## The information contained in this file is the property of CHILI CHIPS LLC.
## Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
## of this file: (1) shall keep all information contained herein confidential;
## and (2) shall protect the same in whole or in part from disclosure and
## dissemination to all third parties; and (3) shall use the same for operation
## and maintenance purposes only.
##--------------------------------------------------------------------------
## Description:
##   Constraints
##==========================================================================

#################################################
##################################################
############## NET - IOSTANDARD ######################
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

#############SPI Configurate Setting##################
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design] 
set_property CONFIG_MODE SPIx4 [current_design] 
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design] 

#                   dodali da provjerimo 
set_property BITSTREAM.CONFIG.CCLK_TRISTATE TRUE [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

############## clock define###########################
create_clock -period 5 [get_ports clk_p]
set_property PACKAGE_PIN R4 [get_ports clk_p]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_p]
create_clock -period 5 [get_ports clk_n]
set_property PACKAGE_PIN T4 [get_ports clk_n]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_n]

############## key define#############################
set_property PACKAGE_PIN T6 [get_ports rst_n]
set_property IOSTANDARD LVCMOS15 [get_ports rst_n]

############ USER KEYS #################################
set_property PACKAGE_PIN J21 [get_ports user_key1]
set_property IOSTANDARD LVCMOS33 [get_ports user_key1]

set_property PACKAGE_PIN E13 [get_ports user_key2]
set_property IOSTANDARD LVCMOS33 [get_ports user_key2]

############### AD ##############################
########ad9238 ON AX7103 J13###########################
set_property PACKAGE_PIN W16  [get_ports {ad9238_clk_ch1}]
set_property PACKAGE_PIN W15  [get_ports {ad9238_data_ch1[0]}]
set_property PACKAGE_PIN V17  [get_ports {ad9238_data_ch1[1]}]
set_property PACKAGE_PIN W17  [get_ports {ad9238_data_ch1[2]}]
set_property PACKAGE_PIN U15  [get_ports {ad9238_data_ch1[3]}]
set_property PACKAGE_PIN V15  [get_ports {ad9238_data_ch1[4]}]
set_property PACKAGE_PIN AB21  [get_ports {ad9238_data_ch1[5]}]
set_property PACKAGE_PIN AB22  [get_ports {ad9238_data_ch1[6]}]
set_property PACKAGE_PIN AA21  [get_ports {ad9238_data_ch1[7]}]
set_property PACKAGE_PIN AA20  [get_ports {ad9238_data_ch1[8]}]
set_property PACKAGE_PIN AB20  [get_ports {ad9238_data_ch1[9]}]
set_property PACKAGE_PIN AA19  [get_ports {ad9238_data_ch1[10]}]
set_property PACKAGE_PIN AA18  [get_ports {ad9238_data_ch1[11]}]

set_property PACKAGE_PIN W22  [get_ports {ad9238_data_ch0[1]}]
set_property PACKAGE_PIN W21  [get_ports {ad9238_data_ch0[0]}]
set_property PACKAGE_PIN T21  [get_ports {ad9238_data_ch0[3]}]
set_property PACKAGE_PIN U21  [get_ports {ad9238_data_ch0[2]}]
set_property PACKAGE_PIN Y21  [get_ports {ad9238_data_ch0[5]}]
set_property PACKAGE_PIN Y22  [get_ports {ad9238_data_ch0[4]}]
set_property PACKAGE_PIN W20  [get_ports {ad9238_data_ch0[7]}]
set_property PACKAGE_PIN W19  [get_ports {ad9238_data_ch0[6]}]
set_property PACKAGE_PIN Y19  [get_ports {ad9238_data_ch0[9]}]
set_property PACKAGE_PIN Y18  [get_ports {ad9238_data_ch0[8]}]
set_property PACKAGE_PIN V22  [get_ports {ad9238_data_ch0[11]}]
set_property PACKAGE_PIN U22  [get_ports {ad9238_data_ch0[10]}]
set_property PACKAGE_PIN T18  [get_ports {ad9238_clk_ch0}]

set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[11]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_clk_ch0}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[9]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[10]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[7]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[8]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[5]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[6]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[3]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[4]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[1]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[2]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch0[0]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[11]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_clk_ch1}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[9]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[10]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[7]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[8]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[5]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[6]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[3]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[4]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[1]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[2]}]
set_property IOSTANDARD LVCMOS33  [get_ports {ad9238_data_ch1[0]}]

set_property SLEW FAST [get_ports {ad9238_clk_ch0}]
set_property SLEW FAST [get_ports {ad9238_clk_ch1}]

set_property IOB true [get_ports {ad9238_data_ch0[11]}]
set_property IOB true [get_ports {ad9238_data_ch0[9]}]
set_property IOB true [get_ports {ad9238_data_ch0[10]}]
set_property IOB true [get_ports {ad9238_data_ch0[7]}]
set_property IOB true [get_ports {ad9238_data_ch0[8]}]
set_property IOB true [get_ports {ad9238_data_ch0[5]}]
set_property IOB true [get_ports {ad9238_data_ch0[6]}]
set_property IOB true [get_ports {ad9238_data_ch0[3]}]
set_property IOB true [get_ports {ad9238_data_ch0[4]}]
set_property IOB true [get_ports {ad9238_data_ch0[1]}]
set_property IOB true [get_ports {ad9238_data_ch0[2]}]
set_property IOB true [get_ports {ad9238_data_ch0[0]}]
set_property IOB true [get_ports {ad9238_data_ch1[11]}]
set_property IOB true [get_ports {ad9238_data_ch1[9]}]
set_property IOB true [get_ports {ad9238_data_ch1[10]}]
set_property IOB true [get_ports {ad9238_data_ch1[7]}]
set_property IOB true [get_ports {ad9238_data_ch1[8]}]
set_property IOB true [get_ports {ad9238_data_ch1[5]}]
set_property IOB true [get_ports {ad9238_data_ch1[6]}]
set_property IOB true [get_ports {ad9238_data_ch1[3]}]
set_property IOB true [get_ports {ad9238_data_ch1[4]}]
set_property IOB true [get_ports {ad9238_data_ch1[1]}]
set_property IOB true [get_ports {ad9238_data_ch1[2]}]
set_property IOB true [get_ports {ad9238_data_ch1[0]}]

##############DA1 define##################
set_property PACKAGE_PIN F13 [get_ports {da1_clk}]
set_property PACKAGE_PIN F14 [get_ports {da1_wrt}]
set_property PACKAGE_PIN AB15 [get_ports {da1_data[13]}]
set_property PACKAGE_PIN AA15 [get_ports {da1_data[12]}]
set_property PACKAGE_PIN AA14 [get_ports {da1_data[11]}]
set_property PACKAGE_PIN Y13 [get_ports {da1_data[10]}]
set_property PACKAGE_PIN AB17 [get_ports {da1_data[9]}]
set_property PACKAGE_PIN AB16 [get_ports {da1_data[8]}]
set_property PACKAGE_PIN AA16 [get_ports {da1_data[7]}]
set_property PACKAGE_PIN Y16 [get_ports {da1_data[6]}]
set_property PACKAGE_PIN AB12 [get_ports {da1_data[5]}]
set_property PACKAGE_PIN AB11 [get_ports {da1_data[4]}]
set_property PACKAGE_PIN Y14 [get_ports {da1_data[3]}]
set_property PACKAGE_PIN W14 [get_ports {da1_data[2]}]
set_property PACKAGE_PIN C19 [get_ports {da1_data[1]}]
set_property PACKAGE_PIN C18 [get_ports {da1_data[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_wrt}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_clk}]

##############J11 on ax7103 #############################
set_property PACKAGE_PIN P16 [get_ports {da1_data[13]}]
set_property PACKAGE_PIN R17 [get_ports {da1_data[12]}]
set_property PACKAGE_PIN R16 [get_ports {da1_data[11]}]
set_property PACKAGE_PIN P15 [get_ports {da1_data[10]}]
set_property PACKAGE_PIN N17 [get_ports {da1_data[9]}]
set_property PACKAGE_PIN P17 [get_ports {da1_data[8]}]
set_property PACKAGE_PIN U16 [get_ports {da1_data[7]}]
set_property PACKAGE_PIN T16 [get_ports {da1_data[6]}]
set_property PACKAGE_PIN U17 [get_ports {da1_data[5]}]
set_property PACKAGE_PIN U18 [get_ports {da1_data[4]}]
set_property PACKAGE_PIN P19 [get_ports {da1_data[3]}]
set_property PACKAGE_PIN R19 [get_ports {da1_data[2]}]
set_property PACKAGE_PIN V18 [get_ports {da1_data[1]}]
set_property PACKAGE_PIN V19 [get_ports {da1_data[0]}]
set_property PACKAGE_PIN U20 [get_ports {da1_wrt}]
set_property PACKAGE_PIN V20 [get_ports {da1_clk}]

set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_data[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_wrt}]
set_property IOSTANDARD LVCMOS33 [get_ports {da1_clk}]

set_property PACKAGE_PIN AA9 [get_ports {da2_clk}]
set_property PACKAGE_PIN AB10 [get_ports {da2_wrt}]
set_property PACKAGE_PIN AA10 [get_ports {da2_data[13]}]
set_property PACKAGE_PIN AA11 [get_ports {da2_data[12]}]
set_property PACKAGE_PIN W10 [get_ports {da2_data[11]}]
set_property PACKAGE_PIN V10 [get_ports {da2_data[10]}]
set_property PACKAGE_PIN Y12 [get_ports {da2_data[9]}]
set_property PACKAGE_PIN Y11 [get_ports {da2_data[8]}]
set_property PACKAGE_PIN W12 [get_ports {da2_data[7]}]
set_property PACKAGE_PIN W11 [get_ports {da2_data[6]}]
set_property PACKAGE_PIN AA15 [get_ports {da2_data[5]}]
set_property PACKAGE_PIN AB15 [get_ports {da2_data[4]}]
set_property PACKAGE_PIN Y16 [get_ports {da2_data[3]}]
set_property PACKAGE_PIN AA16 [get_ports {da2_data[2]}]
set_property PACKAGE_PIN AB16 [get_ports {da2_data[1]}]
set_property PACKAGE_PIN AB17 [get_ports {da2_data[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports {da2_clk}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_wrt}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {da2_data[0]}]

####################### UART CONSTRAINTS ###################
set_property IOSTANDARD LVCMOS33 [get_ports uart_rx]
set_property PACKAGE_PIN P20 [get_ports uart_rx]
set_property IOSTANDARD LVCMOS33 [get_ports uart_tx]
set_property PACKAGE_PIN N15 [get_ports uart_tx]

############# LED SETTINGS ###########################
set_property PACKAGE_PIN B13 [get_ports {led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[0]}]

set_property PACKAGE_PIN C13 [get_ports {led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[1]}]

set_property PACKAGE_PIN D14 [get_ports {led[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[2]}]

set_property PACKAGE_PIN D15 [get_ports {led[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[3]}]
