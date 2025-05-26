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

############## NET - IOSTANDARD ######################
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

#############SPI Configurate Setting##################
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
set_property CONFIG_MODE SPIx4 [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design]
set_property BITSTREAM.CONFIG.CCLK_TRISTATE TRUE [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

############## clock define###########################
create_clock -period 5.000 [get_ports clk_p]
set_property PACKAGE_PIN R4 [get_ports clk_p]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_p]

#################reset setting########################
set_property IOSTANDARD LVCMOS15 [get_ports rst_n]
set_property PACKAGE_PIN T6 [get_ports rst_n]

#############LED Setting###########################
set_property PACKAGE_PIN E17 [get_ports {led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[0]}]

set_property PACKAGE_PIN F16 [get_ports {led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[1]}]

############## key define############################
set_property PACKAGE_PIN D16 [get_ports {key_in[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {key_in[0]}]

set_property PACKAGE_PIN E16 [get_ports {key_in[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {key_in[1]}]

############## usb uart define########################
set_property IOSTANDARD LVCMOS33 [get_ports uart_rx]
set_property PACKAGE_PIN AA15 [get_ports uart_rx]

set_property IOSTANDARD LVCMOS33 [get_ports uart_tx]
set_property PACKAGE_PIN AB15 [get_ports uart_tx]
